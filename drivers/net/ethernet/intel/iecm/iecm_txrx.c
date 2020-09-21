// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Intel Corporation */

#include <linux/net/intel/iecm.h>

/**
 * iecm_buf_lifo_push - push a buffer pointer onto stack
 * @stack: pointer to stack struct
 * @buf: pointer to buf to push
 *
 * Returns 0 on success, negative on failure
 **/
static int iecm_buf_lifo_push(struct iecm_buf_lifo *stack,
			      struct iecm_tx_buf *buf)
{
	if (stack->top == stack->size)
		return -ENOSPC;

	stack->bufs[stack->top++] = buf;

	return 0;
}

/**
 * iecm_buf_lifo_pop - pop a buffer pointer from stack
 * @stack: pointer to stack struct
 **/
static struct iecm_tx_buf *iecm_buf_lifo_pop(struct iecm_buf_lifo *stack)
{
	if (!stack->top)
		return NULL;

	return stack->bufs[--stack->top];
}

/**
 * iecm_get_stats64 - get statistics for network device structure
 * @netdev: network interface device structure
 * @stats: main device statistics structure
 */
void iecm_get_stats64(struct net_device *netdev,
		      struct rtnl_link_stats64 *stats)
{
	struct iecm_vport *vport = iecm_netdev_to_vport(netdev);

	stats->rx_packets = vport->netstats.rx_packets;
	stats->tx_packets = vport->netstats.tx_packets;
	stats->rx_bytes = vport->netstats.rx_bytes;
	stats->tx_bytes = vport->netstats.tx_bytes;
	stats->tx_errors = vport->netstats.tx_errors;
	stats->rx_dropped = vport->netstats.rx_dropped;
	stats->tx_dropped = vport->netstats.tx_dropped;
}

/**
 * iecm_tx_buf_rel - Release a Tx buffer
 * @tx_q: the queue that owns the buffer
 * @tx_buf: the buffer to free
 */
void iecm_tx_buf_rel(struct iecm_queue *tx_q, struct iecm_tx_buf *tx_buf)
{
	if (tx_buf->skb) {
		dev_kfree_skb_any(tx_buf->skb);
		if (dma_unmap_len(tx_buf, len))
			dma_unmap_single(tx_q->dev,
					 dma_unmap_addr(tx_buf, dma),
					 dma_unmap_len(tx_buf, len),
					 DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buf, len)) {
		dma_unmap_page(tx_q->dev,
			       dma_unmap_addr(tx_buf, dma),
			       dma_unmap_len(tx_buf, len),
			       DMA_TO_DEVICE);
	}

	tx_buf->next_to_watch = NULL;
	tx_buf->skb = NULL;
	dma_unmap_len_set(tx_buf, len, 0);
}

/**
 * iecm_tx_buf_rel all - Free any empty Tx buffers
 * @txq: queue to be cleaned
 */
static void iecm_tx_buf_rel_all(struct iecm_queue *txq)
{
	u16 i;

	/* Buffers already cleared, nothing to do */
	if (!txq->tx_buf)
		return;

	/* Free all the Tx buffer sk_buffs */
	for (i = 0; i < txq->desc_count; i++)
		iecm_tx_buf_rel(txq, &txq->tx_buf[i]);

	kfree(txq->tx_buf);
	txq->tx_buf = NULL;

	if (txq->buf_stack.bufs) {
		for (i = 0; i < txq->buf_stack.size; i++) {
			iecm_tx_buf_rel(txq, txq->buf_stack.bufs[i]);
			kfree(txq->buf_stack.bufs[i]);
		}
		kfree(txq->buf_stack.bufs);
	}
}

/**
 * iecm_tx_desc_rel - Free Tx resources per queue
 * @txq: Tx descriptor ring for a specific queue
 * @bufq: buffer q or completion q
 *
 * Free all transmit software resources
 */
static void iecm_tx_desc_rel(struct iecm_queue *txq, bool bufq)
{
	if (bufq)
		iecm_tx_buf_rel_all(txq);

	if (txq->desc_ring) {
		dmam_free_coherent(txq->dev, txq->size,
				   txq->desc_ring, txq->dma);
		txq->desc_ring = NULL;
		txq->next_to_alloc = 0;
		txq->next_to_use = 0;
		txq->next_to_clean = 0;
	}
}

/**
 * iecm_tx_desc_rel_all - Free Tx Resources for All Queues
 * @vport: virtual port structure
 *
 * Free all transmit software resources
 */
static void iecm_tx_desc_rel_all(struct iecm_vport *vport)
{
	struct iecm_queue *txq;
	int i, j;

	if (!vport->txq_grps)
		return;

	for (i = 0; i < vport->num_txq_grp; i++) {
		for (j = 0; j < vport->txq_grps[i].num_txq; j++) {
			if (vport->txq_grps[i].txqs) {
				txq = &vport->txq_grps[i].txqs[j];
				iecm_tx_desc_rel(txq, true);
			}
		}
		if (iecm_is_queue_model_split(vport->txq_model)) {
			txq = vport->txq_grps[i].complq;
			iecm_tx_desc_rel(txq, false);
		}
	}
}

/**
 * iecm_tx_buf_alloc_all - Allocate memory for all buffer resources
 * @tx_q: queue for which the buffers are allocated
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_tx_buf_alloc_all(struct iecm_queue *tx_q)
{
	int buf_size;
	int i = 0;

	/* Allocate book keeping buffers only. Buffers to be supplied to HW
	 * are allocated by kernel network stack and received as part of skb
	 */
	buf_size = sizeof(struct iecm_tx_buf) * tx_q->desc_count;
	tx_q->tx_buf = kzalloc(buf_size, GFP_KERNEL);
	if (!tx_q->tx_buf)
		return -ENOMEM;

	/* Initialize Tx buf stack for out-of-order completions if
	 * flow scheduling offload is enabled
	 */
	tx_q->buf_stack.bufs =
		kcalloc(tx_q->desc_count, sizeof(struct iecm_tx_buf *),
			GFP_KERNEL);
	if (!tx_q->buf_stack.bufs)
		return -ENOMEM;

	for (i = 0; i < tx_q->desc_count; i++) {
		tx_q->buf_stack.bufs[i] =
			kzalloc(sizeof(*tx_q->buf_stack.bufs[i]),
				GFP_KERNEL);
		if (!tx_q->buf_stack.bufs[i])
			return -ENOMEM;
	}

	tx_q->buf_stack.size = tx_q->desc_count;
	tx_q->buf_stack.top = tx_q->desc_count;

	return 0;
}

/**
 * iecm_tx_desc_alloc - Allocate the Tx descriptors
 * @tx_q: the Tx ring to set up
 * @bufq: buffer or completion queue
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_tx_desc_alloc(struct iecm_queue *tx_q, bool bufq)
{
	struct device *dev = tx_q->dev;
	int err = 0;

	if (bufq) {
		err = iecm_tx_buf_alloc_all(tx_q);
		if (err)
			goto err_alloc;
		tx_q->size = tx_q->desc_count *
				sizeof(struct iecm_base_tx_desc);
	} else {
		tx_q->size = tx_q->desc_count *
				sizeof(struct iecm_splitq_tx_compl_desc);
	}

	/* Allocate descriptors also round up to nearest 4K */
	tx_q->size = ALIGN(tx_q->size, 4096);
	tx_q->desc_ring = dmam_alloc_coherent(dev, tx_q->size, &tx_q->dma,
					      GFP_KERNEL);
	if (!tx_q->desc_ring) {
		dev_info(dev, "Unable to allocate memory for the Tx descriptor ring, size=%d\n",
			 tx_q->size);
		err = -ENOMEM;
		goto err_alloc;
	}

	tx_q->next_to_alloc = 0;
	tx_q->next_to_use = 0;
	tx_q->next_to_clean = 0;
	set_bit(__IECM_Q_GEN_CHK, tx_q->flags);

err_alloc:
	if (err)
		iecm_tx_desc_rel(tx_q, bufq);
	return err;
}

/**
 * iecm_tx_desc_alloc_all - allocate all queues Tx resources
 * @vport: virtual port private structure
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_tx_desc_alloc_all(struct iecm_vport *vport)
{
	struct pci_dev *pdev = vport->adapter->pdev;
	int err = 0;
	int i, j;

	/* Setup buffer queues. In single queue model buffer queues and
	 * completion queues will be same
	 */
	for (i = 0; i < vport->num_txq_grp; i++) {
		for (j = 0; j < vport->txq_grps[i].num_txq; j++) {
			err = iecm_tx_desc_alloc(&vport->txq_grps[i].txqs[j],
						 true);
			if (err) {
				dev_err(&pdev->dev,
					"Allocation for Tx Queue %u failed\n",
					i);
				goto err_out;
			}
		}

		if (iecm_is_queue_model_split(vport->txq_model)) {
			/* Setup completion queues */
			err = iecm_tx_desc_alloc(vport->txq_grps[i].complq,
						 false);
			if (err) {
				dev_err(&pdev->dev,
					"Allocation for Tx Completion Queue %u failed\n",
					i);
				goto err_out;
			}
		}
	}
err_out:
	if (err)
		iecm_tx_desc_rel_all(vport);
	return err;
}

/**
 * iecm_rx_buf_rel - Release a Rx buffer
 * @rxq: the queue that owns the buffer
 * @rx_buf: the buffer to free
 */
static void iecm_rx_buf_rel(struct iecm_queue *rxq,
			    struct iecm_rx_buf *rx_buf)
{
	struct device *dev = rxq->dev;

	if (!rx_buf->page)
		return;

	if (rx_buf->skb) {
		dev_kfree_skb_any(rx_buf->skb);
		rx_buf->skb = NULL;
	}

	dma_unmap_page(dev, rx_buf->dma, PAGE_SIZE, DMA_FROM_DEVICE);
	__free_pages(rx_buf->page, 0);

	rx_buf->page = NULL;
	rx_buf->page_offset = 0;
}

/**
 * iecm_rx_buf_rel_all - Free all Rx buffer resources for a queue
 * @rxq: queue to be cleaned
 */
static void iecm_rx_buf_rel_all(struct iecm_queue *rxq)
{
	u16 i;

	/* queue already cleared, nothing to do */
	if (!rxq->rx_buf.buf)
		return;

	/* Free all the bufs allocated and given to HW on Rx queue */
	for (i = 0; i < rxq->desc_count; i++) {
		iecm_rx_buf_rel(rxq, &rxq->rx_buf.buf[i]);
		if (rxq->rx_hsplit_en)
			iecm_rx_buf_rel(rxq, &rxq->rx_buf.hdr_buf[i]);
	}

	kfree(rxq->rx_buf.buf);
	rxq->rx_buf.buf = NULL;
	kfree(rxq->rx_buf.hdr_buf);
	rxq->rx_buf.hdr_buf = NULL;
}

/**
 * iecm_rx_desc_rel - Free a specific Rx q resources
 * @rxq: queue to clean the resources from
 * @bufq: buffer q or completion q
 * @q_model: single or split q model
 *
 * Free a specific Rx queue resources
 */
static void iecm_rx_desc_rel(struct iecm_queue *rxq, bool bufq,
			     enum virtchnl_queue_model q_model)
{
	if (!rxq)
		return;

	if (!bufq && iecm_is_queue_model_split(q_model) && rxq->skb) {
		dev_kfree_skb_any(rxq->skb);
		rxq->skb = NULL;
	}

	if (bufq || !iecm_is_queue_model_split(q_model))
		iecm_rx_buf_rel_all(rxq);

	if (rxq->desc_ring) {
		dmam_free_coherent(rxq->dev, rxq->size,
				   rxq->desc_ring, rxq->dma);
		rxq->desc_ring = NULL;
		rxq->next_to_alloc = 0;
		rxq->next_to_clean = 0;
		rxq->next_to_use = 0;
	}
}

/**
 * iecm_rx_desc_rel_all - Free Rx Resources for All Queues
 * @vport: virtual port structure
 *
 * Free all Rx queues resources
 */
static void iecm_rx_desc_rel_all(struct iecm_vport *vport)
{
	struct iecm_rxq_group *rx_qgrp;
	struct iecm_queue *q;
	int i, j, num_rxq;

	if (!vport->rxq_grps)
		return;

	for (i = 0; i < vport->num_rxq_grp; i++) {
		rx_qgrp = &vport->rxq_grps[i];

		if (iecm_is_queue_model_split(vport->rxq_model)) {
			if (rx_qgrp->splitq.rxq_sets) {
				num_rxq = rx_qgrp->splitq.num_rxq_sets;
				for (j = 0; j < num_rxq; j++) {
					q = &rx_qgrp->splitq.rxq_sets[j].rxq;
					iecm_rx_desc_rel(q, false,
							 vport->rxq_model);
				}
			}

			if (!rx_qgrp->splitq.bufq_sets)
				continue;
			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++) {
				struct iecm_bufq_set *bufq_set =
					&rx_qgrp->splitq.bufq_sets[j];

				q = &bufq_set->bufq;
				iecm_rx_desc_rel(q, true, vport->rxq_model);
				if (!bufq_set->refillqs)
					continue;
				kfree(bufq_set->refillqs);
				bufq_set->refillqs = NULL;
			}
		} else {
			if (rx_qgrp->singleq.rxqs) {
				for (j = 0; j < rx_qgrp->singleq.num_rxq; j++) {
					q = &rx_qgrp->singleq.rxqs[j];
					iecm_rx_desc_rel(q, false,
							 vport->rxq_model);
				}
			}
		}
	}
}

/**
 * iecm_rx_buf_hw_update - Store the new tail and head values
 * @rxq: queue to bump
 * @val: new head index
 */
void iecm_rx_buf_hw_update(struct iecm_queue *rxq, u32 val)
{
	/* update next to alloc since we have filled the ring */
	rxq->next_to_alloc = val;

	rxq->next_to_use = val;
	if (!rxq->tail)
		return;
	/* Force memory writes to complete before letting h/w
	 * know there are new descriptors to fetch.  (Only
	 * applicable for weak-ordered memory model archs,
	 * such as IA-64).
	 */
	wmb();
	writel(val, rxq->tail);
}

/**
 * iecm_rx_buf_hw_alloc - recycle or make a new page
 * @rxq: ring to use
 * @buf: rx_buffer struct to modify
 *
 * Returns true if the page was successfully allocated or
 * reused.
 */
bool iecm_rx_buf_hw_alloc(struct iecm_queue *rxq, struct iecm_rx_buf *buf)
{
	struct page *page = buf->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	/* alloc new page for storage */
	page = alloc_page(GFP_ATOMIC | __GFP_NOWARN);
	if (unlikely(!page))
		return false;

	/* map page for use */
	dma = dma_map_page(rxq->dev, page, 0, PAGE_SIZE, DMA_FROM_DEVICE);

	/* if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rxq->dev, dma)) {
		__free_pages(page, 0);
		return false;
	}

	buf->dma = dma;
	buf->page = page;
	buf->page_offset = iecm_rx_offset(rxq);

	return true;
}

/**
 * iecm_rx_hdr_buf_hw_alloc - recycle or make a new page for header buffer
 * @rxq: ring to use
 * @hdr_buf: rx_buffer struct to modify
 *
 * Returns true if the page was successfully allocated or
 * reused.
 */
static bool iecm_rx_hdr_buf_hw_alloc(struct iecm_queue *rxq,
				     struct iecm_rx_buf *hdr_buf)
{
	struct page *page = hdr_buf->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	/* alloc new page for storage */
	page = alloc_page(GFP_ATOMIC | __GFP_NOWARN);
	if (unlikely(!page))
		return false;

	/* map page for use */
	dma = dma_map_page(rxq->dev, page, 0, PAGE_SIZE, DMA_FROM_DEVICE);

	/* if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rxq->dev, dma)) {
		__free_pages(page, 0);
		return false;
	}

	hdr_buf->dma = dma;
	hdr_buf->page = page;
	hdr_buf->page_offset = 0;

	return true;
}

/**
 * iecm_rx_buf_hw_alloc_all - Replace used receive buffers
 * @rxq: queue for which the hw buffers are allocated
 * @cleaned_count: number of buffers to replace
 *
 * Returns false if all allocations were successful, true if any fail
 */
static bool
iecm_rx_buf_hw_alloc_all(struct iecm_queue *rxq,
			 u16 cleaned_count)
{
	struct iecm_splitq_rx_buf_desc *splitq_rx_desc = NULL;
	struct iecm_rx_buf *hdr_buf = NULL;
	u16 nta = rxq->next_to_alloc;
	struct iecm_rx_buf *buf;

	/* do nothing if no valid netdev defined */
	if (!rxq->vport->netdev || !cleaned_count)
		return false;

	splitq_rx_desc = IECM_SPLITQ_RX_BUF_DESC(rxq, nta);

	buf = &rxq->rx_buf.buf[nta];
	if (rxq->rx_hsplit_en)
		hdr_buf = &rxq->rx_buf.hdr_buf[nta];

	do {
		if (rxq->rx_hsplit_en) {
			if (!iecm_rx_hdr_buf_hw_alloc(rxq, hdr_buf))
				break;

			splitq_rx_desc->hdr_addr =
				cpu_to_le64(hdr_buf->dma +
					    hdr_buf->page_offset);
			hdr_buf++;
		}

		if (!iecm_rx_buf_hw_alloc(rxq, buf))
			break;

		/* Refresh the desc even if buffer_addrs didn't change
		 * because each write-back erases this info.
		 */
		splitq_rx_desc->pkt_addr =
			cpu_to_le64(buf->dma + buf->page_offset);
		splitq_rx_desc->qword0.buf_id = cpu_to_le16(nta);

		splitq_rx_desc++;
		buf++;
		nta++;
		if (unlikely(nta == rxq->desc_count)) {
			splitq_rx_desc = IECM_SPLITQ_RX_BUF_DESC(rxq, 0);
			buf = rxq->rx_buf.buf;
			hdr_buf = rxq->rx_buf.hdr_buf;
			nta = 0;
		}

		cleaned_count--;
	} while (cleaned_count);

	if (rxq->next_to_alloc != nta)
		iecm_rx_buf_hw_update(rxq, nta);

	return !!cleaned_count;
}

/**
 * iecm_rx_buf_alloc_all - Allocate memory for all buffer resources
 * @rxq: queue for which the buffers are allocated
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_rx_buf_alloc_all(struct iecm_queue *rxq)
{
	int err = 0;

	/* Allocate book keeping buffers */
	rxq->rx_buf.buf = kcalloc(rxq->desc_count, sizeof(struct iecm_rx_buf),
				  GFP_KERNEL);
	if (!rxq->rx_buf.buf) {
		err = -ENOMEM;
		goto rx_buf_alloc_all_out;
	}

	if (rxq->rx_hsplit_en) {
		rxq->rx_buf.hdr_buf =
			kcalloc(rxq->desc_count, sizeof(struct iecm_rx_buf),
				GFP_KERNEL);
		if (!rxq->rx_buf.hdr_buf) {
			err = -ENOMEM;
			goto rx_buf_alloc_all_out;
		}
	} else {
		rxq->rx_buf.hdr_buf = NULL;
	}

	/* Allocate buffers to be given to HW. Allocate one less than
	 * total descriptor count as RX splits 4k buffers to 2K and recycles
	 */
	if (iecm_is_queue_model_split(rxq->vport->rxq_model)) {
		if (iecm_rx_buf_hw_alloc_all(rxq,
					     rxq->desc_count - 1))
			err = -ENOMEM;
	} else if (iecm_rx_singleq_buf_hw_alloc_all(rxq,
						    rxq->desc_count - 1)) {
		err = -ENOMEM;
	}

rx_buf_alloc_all_out:
	if (err)
		iecm_rx_buf_rel_all(rxq);
	return err;
}

/**
 * iecm_rx_desc_alloc - Allocate queue Rx resources
 * @rxq: Rx queue for which the resources are setup
 * @bufq: buffer or completion queue
 * @q_model: single or split queue model
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_rx_desc_alloc(struct iecm_queue *rxq, bool bufq,
			      enum virtchnl_queue_model q_model)
{
	struct device *dev = rxq->dev;

	/* As both single and split descriptors are 32 byte, memory size
	 * will be same for all three singleq_base Rx, buf., splitq_base
	 * Rx. So pick anyone of them for size
	 */
	if (bufq) {
		rxq->size = rxq->desc_count *
			sizeof(struct iecm_splitq_rx_buf_desc);
	} else {
		rxq->size = rxq->desc_count *
			sizeof(union iecm_rx_desc);
	}

	/* Allocate descriptors and also round up to nearest 4K */
	rxq->size = ALIGN(rxq->size, 4096);
	rxq->desc_ring = dmam_alloc_coherent(dev, rxq->size,
					     &rxq->dma, GFP_KERNEL);
	if (!rxq->desc_ring) {
		dev_info(dev, "Unable to allocate memory for the Rx descriptor ring, size=%d\n",
			 rxq->size);
		return -ENOMEM;
	}

	rxq->next_to_alloc = 0;
	rxq->next_to_clean = 0;
	rxq->next_to_use = 0;
	set_bit(__IECM_Q_GEN_CHK, rxq->flags);

	/* Allocate buffers for a Rx queue if the q_model is single OR if it
	 * is a buffer queue in split queue model
	 */
	if (bufq || !iecm_is_queue_model_split(q_model)) {
		int err = 0;

		err = iecm_rx_buf_alloc_all(rxq);
		if (err) {
			iecm_rx_desc_rel(rxq, bufq, q_model);
			return err;
		}
	}
	return 0;
}

/**
 * iecm_rx_desc_alloc_all - allocate all RX queues resources
 * @vport: virtual port structure
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_rx_desc_alloc_all(struct iecm_vport *vport)
{
	struct device *dev = &vport->adapter->pdev->dev;
	struct iecm_queue *q;
	int i, j, num_rxq;
	int err = 0;

	for (i = 0; i < vport->num_rxq_grp; i++) {
		if (iecm_is_queue_model_split(vport->rxq_model))
			num_rxq = vport->rxq_grps[i].splitq.num_rxq_sets;
		else
			num_rxq = vport->rxq_grps[i].singleq.num_rxq;

		for (j = 0; j < num_rxq; j++) {
			if (iecm_is_queue_model_split(vport->rxq_model))
				q = &vport->rxq_grps[i].splitq.rxq_sets[j].rxq;
			else
				q = &vport->rxq_grps[i].singleq.rxqs[j];
			err = iecm_rx_desc_alloc(q, false, vport->rxq_model);
			if (err) {
				dev_err(dev, "Memory allocation for Rx Queue %u failed\n",
					i);
				goto err_out;
			}
		}

		if (iecm_is_queue_model_split(vport->rxq_model)) {
			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++) {
				q =
				  &vport->rxq_grps[i].splitq.bufq_sets[j].bufq;
				err = iecm_rx_desc_alloc(q, true,
							 vport->rxq_model);
				if (err) {
					dev_err(dev, "Memory allocation for Rx Buffer Queue %u failed\n",
						i);
					goto err_out;
				}
			}
		}
	}
err_out:
	if (err)
		iecm_rx_desc_rel_all(vport);
	return err;
}

/**
 * iecm_txq_group_rel - Release all resources for txq groups
 * @vport: vport to release txq groups on
 */
static void iecm_txq_group_rel(struct iecm_vport *vport)
{
	if (vport->txq_grps) {
		int i;

		for (i = 0; i < vport->num_txq_grp; i++) {
			kfree(vport->txq_grps[i].txqs);
			vport->txq_grps[i].txqs = NULL;
			kfree(vport->txq_grps[i].complq);
			vport->txq_grps[i].complq = NULL;
		}
		kfree(vport->txq_grps);
		vport->txq_grps = NULL;
	}
}

/**
 * iecm_rxq_group_rel - Release all resources for rxq groups
 * @vport: vport to release rxq groups on
 */
static void iecm_rxq_group_rel(struct iecm_vport *vport)
{
	if (vport->rxq_grps) {
		int i;

		for (i = 0; i < vport->num_rxq_grp; i++) {
			struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];

			if (iecm_is_queue_model_split(vport->rxq_model)) {
				kfree(rx_qgrp->splitq.rxq_sets);
				rx_qgrp->splitq.rxq_sets = NULL;
				kfree(rx_qgrp->splitq.bufq_sets);
				rx_qgrp->splitq.bufq_sets = NULL;
			} else {
				kfree(rx_qgrp->singleq.rxqs);
				vport->rxq_grps[i].singleq.rxqs = NULL;
			}
		}
		kfree(vport->rxq_grps);
		vport->rxq_grps = NULL;
	}
}

/**
 * iecm_vport_queue_grp_rel_all - Release all queue groups
 * @vport: vport to release queue groups for
 */
static void iecm_vport_queue_grp_rel_all(struct iecm_vport *vport)
{
	iecm_txq_group_rel(vport);
	iecm_rxq_group_rel(vport);
}

/**
 * iecm_vport_queues_rel - Free memory for all queues
 * @vport: virtual port
 *
 * Free the memory allocated for queues associated to a vport
 */
void iecm_vport_queues_rel(struct iecm_vport *vport)
{
	iecm_tx_desc_rel_all(vport);
	iecm_rx_desc_rel_all(vport);
	iecm_vport_queue_grp_rel_all(vport);

	kfree(vport->txqs);
	vport->txqs = NULL;
}

/**
 * iecm_vport_init_fast_path_txqs - Initialize fast path txq array
 * @vport: vport to init txqs on
 *
 * We get a queue index from skb->queue_mapping and we need a fast way to
 * dereference the queue from queue groups.  This allows us to quickly pull a
 * txq based on a queue index.
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_vport_init_fast_path_txqs(struct iecm_vport *vport)
{
	int i, j, k = 0;

	vport->txqs = kcalloc(vport->num_txq, sizeof(struct iecm_queue *),
			      GFP_KERNEL);

	if (!vport->txqs)
		return -ENOMEM;

	for (i = 0; i < vport->num_txq_grp; i++) {
		struct iecm_txq_group *tx_grp = &vport->txq_grps[i];

		for (j = 0; j < tx_grp->num_txq; j++, k++) {
			vport->txqs[k] = &tx_grp->txqs[j];
			vport->txqs[k]->idx = k;
		}
	}
	return 0;
}

/**
 * iecm_vport_init_num_qs - Initialize number of queues
 * @vport: vport to initialize qs
 * @vport_msg: data to be filled into vport
 */
void iecm_vport_init_num_qs(struct iecm_vport *vport,
			    struct virtchnl_create_vport *vport_msg)
{
	vport->num_txq = vport_msg->num_tx_q;
	vport->num_rxq = vport_msg->num_rx_q;
	if (iecm_is_queue_model_split(vport->txq_model))
		vport->num_complq = vport_msg->num_tx_complq;
	if (iecm_is_queue_model_split(vport->rxq_model))
		vport->num_bufq = vport_msg->num_rx_bufq;
}

/**
 * iecm_vport_calc_num_q_desc - Calculate number of queue groups
 * @vport: vport to calculate q groups for
 */
void iecm_vport_calc_num_q_desc(struct iecm_vport *vport)
{
	int num_req_txq_desc = vport->adapter->config_data.num_req_txq_desc;
	int num_req_rxq_desc = vport->adapter->config_data.num_req_rxq_desc;

	vport->complq_desc_count = 0;
	vport->bufq_desc_count = 0;
	if (num_req_txq_desc) {
		vport->txq_desc_count = num_req_txq_desc;
		if (iecm_is_queue_model_split(vport->txq_model))
			vport->complq_desc_count = num_req_txq_desc;
	} else {
		vport->txq_desc_count =
			IECM_DFLT_TX_Q_DESC_COUNT;
		if (iecm_is_queue_model_split(vport->txq_model)) {
			vport->complq_desc_count =
				IECM_DFLT_TX_COMPLQ_DESC_COUNT;
		}
	}
	if (num_req_rxq_desc) {
		vport->rxq_desc_count = num_req_rxq_desc;
		if (iecm_is_queue_model_split(vport->rxq_model))
			vport->bufq_desc_count = num_req_rxq_desc;
	} else {
		vport->rxq_desc_count = IECM_DFLT_RX_Q_DESC_COUNT;
		if (iecm_is_queue_model_split(vport->rxq_model))
			vport->bufq_desc_count = IECM_DFLT_RX_BUFQ_DESC_COUNT;
	}
}
EXPORT_SYMBOL(iecm_vport_calc_num_q_desc);

/**
 * iecm_vport_calc_total_qs - Calculate total number of queues
 * @adapter: private data struct
 * @vport_msg: message to fill with data
 */
void iecm_vport_calc_total_qs(struct iecm_adapter *adapter,
			      struct virtchnl_create_vport *vport_msg)
{
	unsigned int num_req_tx_qs = adapter->config_data.num_req_tx_qs;
	unsigned int num_req_rx_qs = adapter->config_data.num_req_rx_qs;
	int dflt_splitq_txq_grps, dflt_singleq_txqs;
	int dflt_splitq_rxq_grps, dflt_singleq_rxqs;
	int num_txq_grps, num_rxq_grps;
	int num_cpus;

	/* Restrict num of queues to cpus online as a default configuration to
	 * give best performance. User can always override to a max number
	 * of queues via ethtool.
	 */
	num_cpus = num_online_cpus();
	dflt_splitq_txq_grps = min_t(int, IECM_DFLT_SPLITQ_TX_Q_GROUPS,
				     num_cpus);
	dflt_singleq_txqs = min_t(int, IECM_DFLT_SINGLEQ_TXQ_PER_GROUP,
				  num_cpus);
	dflt_splitq_rxq_grps = min_t(int, IECM_DFLT_SPLITQ_RX_Q_GROUPS,
				     num_cpus);
	dflt_singleq_rxqs = min_t(int, IECM_DFLT_SINGLEQ_RXQ_PER_GROUP,
				  num_cpus);

	if (iecm_is_queue_model_split(vport_msg->txq_model)) {
		num_txq_grps = num_req_tx_qs ? num_req_tx_qs : dflt_splitq_txq_grps;
		vport_msg->num_tx_complq = num_txq_grps *
			IECM_COMPLQ_PER_GROUP;
		vport_msg->num_tx_q = num_txq_grps *
				      IECM_DFLT_SPLITQ_TXQ_PER_GROUP;
	} else {
		num_txq_grps = IECM_DFLT_SINGLEQ_TX_Q_GROUPS;
		vport_msg->num_tx_q = num_txq_grps *
				      (num_req_tx_qs ? num_req_tx_qs :
				       dflt_singleq_txqs);
		vport_msg->num_tx_complq = 0;
	}
	if (iecm_is_queue_model_split(vport_msg->rxq_model)) {
		num_rxq_grps = num_req_rx_qs ? num_req_rx_qs : dflt_splitq_rxq_grps;
		vport_msg->num_rx_bufq = num_rxq_grps *
					 IECM_BUFQS_PER_RXQ_SET;
		vport_msg->num_rx_q = num_rxq_grps *
				      IECM_DFLT_SPLITQ_RXQ_PER_GROUP;
	} else {
		num_rxq_grps = IECM_DFLT_SINGLEQ_RX_Q_GROUPS;
		vport_msg->num_rx_bufq = 0;
		vport_msg->num_rx_q = num_rxq_grps *
				      (num_req_rx_qs ? num_req_rx_qs :
				       dflt_singleq_rxqs);
	}
}

/**
 * iecm_vport_calc_num_q_groups - Calculate number of queue groups
 * @vport: vport to calculate q groups for
 */
void iecm_vport_calc_num_q_groups(struct iecm_vport *vport)
{
	if (iecm_is_queue_model_split(vport->txq_model))
		vport->num_txq_grp = vport->num_txq;
	else
		vport->num_txq_grp = IECM_DFLT_SINGLEQ_TX_Q_GROUPS;

	if (iecm_is_queue_model_split(vport->rxq_model))
		vport->num_rxq_grp = vport->num_rxq;
	else
		vport->num_rxq_grp = IECM_DFLT_SINGLEQ_RX_Q_GROUPS;
}
EXPORT_SYMBOL(iecm_vport_calc_num_q_groups);

/**
 * iecm_vport_calc_numq_per_grp - Calculate number of queues per group
 * @vport: vport to calculate queues for
 * @num_txq: int return parameter
 * @num_rxq: int return parameter
 */
static void iecm_vport_calc_numq_per_grp(struct iecm_vport *vport,
					 int *num_txq, int *num_rxq)
{
	if (iecm_is_queue_model_split(vport->txq_model))
		*num_txq = IECM_DFLT_SPLITQ_TXQ_PER_GROUP;
	else
		*num_txq = vport->num_txq;

	if (iecm_is_queue_model_split(vport->rxq_model))
		*num_rxq = IECM_DFLT_SPLITQ_RXQ_PER_GROUP;
	else
		*num_rxq = vport->num_rxq;
}

/**
 * iecm_vport_calc_num_q_vec - Calculate total number of vectors required for
 * this vport
 * @vport: virtual port
 *
 */
void iecm_vport_calc_num_q_vec(struct iecm_vport *vport)
{
	if (iecm_is_queue_model_split(vport->txq_model))
		vport->num_q_vectors = vport->num_txq_grp;
	else
		vport->num_q_vectors = vport->num_txq;
}

/**
 * iecm_txq_group_alloc - Allocate all txq group resources
 * @vport: vport to allocate txq groups for
 * @num_txq: number of txqs to allocate for each group
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_txq_group_alloc(struct iecm_vport *vport, int num_txq)
{
	struct iecm_itr tx_itr = { 0 };
	int err = 0;
	int i;

	vport->txq_grps = kcalloc(vport->num_txq_grp,
				  sizeof(*vport->txq_grps), GFP_KERNEL);
	if (!vport->txq_grps)
		return -ENOMEM;

	tx_itr.target_itr = IECM_ITR_TX_DEF;
	tx_itr.itr_idx = VIRTCHNL_ITR_IDX_1;
	tx_itr.next_update = jiffies + 1;

	for (i = 0; i < vport->num_txq_grp; i++) {
		struct iecm_txq_group *tx_qgrp = &vport->txq_grps[i];
		int j;

		tx_qgrp->vport = vport;
		tx_qgrp->num_txq = num_txq;
		tx_qgrp->txqs = kcalloc(num_txq, sizeof(*tx_qgrp->txqs),
					GFP_KERNEL);
		if (!tx_qgrp->txqs) {
			err = -ENOMEM;
			goto err_alloc;
		}

		for (j = 0; j < tx_qgrp->num_txq; j++) {
			struct iecm_queue *q = &tx_qgrp->txqs[j];

			q->dev = &vport->adapter->pdev->dev;
			q->desc_count = vport->txq_desc_count;
			q->vport = vport;
			q->txq_grp = tx_qgrp;
			hash_init(q->sched_buf_hash);

			if (!iecm_is_queue_model_split(vport->txq_model))
				q->itr = tx_itr;
		}

		if (!iecm_is_queue_model_split(vport->txq_model))
			continue;

		tx_qgrp->complq = kcalloc(IECM_COMPLQ_PER_GROUP,
					  sizeof(*tx_qgrp->complq),
					  GFP_KERNEL);
		if (!tx_qgrp->complq) {
			err = -ENOMEM;
			goto err_alloc;
		}

		tx_qgrp->complq->dev = &vport->adapter->pdev->dev;
		tx_qgrp->complq->desc_count = vport->complq_desc_count;
		tx_qgrp->complq->vport = vport;
		tx_qgrp->complq->txq_grp = tx_qgrp;

		tx_qgrp->complq->itr = tx_itr;
	}

err_alloc:
	if (err)
		iecm_txq_group_rel(vport);
	return err;
}

/**
 * iecm_rxq_group_alloc - Allocate all rxq group resources
 * @vport: vport to allocate rxq groups for
 * @num_rxq: number of rxqs to allocate for each group
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_rxq_group_alloc(struct iecm_vport *vport, int num_rxq)
{
	struct iecm_itr rx_itr = {0};
	struct iecm_queue *q;
	int i, err = 0;

	vport->rxq_grps = kcalloc(vport->num_rxq_grp,
				  sizeof(struct iecm_rxq_group), GFP_KERNEL);
	if (!vport->rxq_grps)
		return -ENOMEM;

	rx_itr.target_itr = IECM_ITR_RX_DEF;
	rx_itr.itr_idx = VIRTCHNL_ITR_IDX_0;
	rx_itr.next_update = jiffies + 1;

	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];
		int j;

		rx_qgrp->vport = vport;
		if (iecm_is_queue_model_split(vport->rxq_model)) {
			rx_qgrp->splitq.num_rxq_sets = num_rxq;
			rx_qgrp->splitq.rxq_sets =
				kcalloc(num_rxq,
					sizeof(struct iecm_rxq_set),
					GFP_KERNEL);
			if (!rx_qgrp->splitq.rxq_sets) {
				err = -ENOMEM;
				goto err_alloc;
			}

			rx_qgrp->splitq.bufq_sets =
				kcalloc(IECM_BUFQS_PER_RXQ_SET,
					sizeof(struct iecm_bufq_set),
					GFP_KERNEL);
			if (!rx_qgrp->splitq.bufq_sets) {
				err = -ENOMEM;
				goto err_alloc;
			}

			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++) {
				int swq_size = sizeof(struct iecm_sw_queue);

				q = &rx_qgrp->splitq.bufq_sets[j].bufq;
				q->dev = &vport->adapter->pdev->dev;
				q->desc_count = vport->bufq_desc_count;
				q->vport = vport;
				q->rxq_grp = rx_qgrp;
				q->idx = j;
				q->rx_buf_size = IECM_RX_BUF_2048;
				q->rsc_low_watermark = IECM_LOW_WATERMARK;
				q->rx_buf_stride = IECM_RX_BUF_STRIDE;
				q->itr = rx_itr;

				if (vport->rx_hsplit_en) {
					q->rx_hsplit_en = vport->rx_hsplit_en;
					q->rx_hbuf_size = IECM_HDR_BUF_SIZE;
				}

				rx_qgrp->splitq.bufq_sets[j].num_refillqs =
					num_rxq;
				rx_qgrp->splitq.bufq_sets[j].refillqs =
					kcalloc(num_rxq, swq_size, GFP_KERNEL);
				if (!rx_qgrp->splitq.bufq_sets[j].refillqs) {
					err = -ENOMEM;
					goto err_alloc;
				}
			}
		} else {
			rx_qgrp->singleq.num_rxq = num_rxq;
			rx_qgrp->singleq.rxqs = kcalloc(num_rxq,
							sizeof(struct iecm_queue),
							GFP_KERNEL);
			if (!rx_qgrp->singleq.rxqs)  {
				err = -ENOMEM;
				goto err_alloc;
			}
		}

		for (j = 0; j < num_rxq; j++) {
			if (iecm_is_queue_model_split(vport->rxq_model)) {
				q = &rx_qgrp->splitq.rxq_sets[j].rxq;
				rx_qgrp->splitq.rxq_sets[j].refillq0 =
				      &rx_qgrp->splitq.bufq_sets[0].refillqs[j];
				rx_qgrp->splitq.rxq_sets[j].refillq1 =
				      &rx_qgrp->splitq.bufq_sets[1].refillqs[j];

				if (vport->rx_hsplit_en) {
					q->rx_hsplit_en = vport->rx_hsplit_en;
					q->rx_hbuf_size = IECM_HDR_BUF_SIZE;
				}

			} else {
				q = &rx_qgrp->singleq.rxqs[j];
			}
			q->dev = &vport->adapter->pdev->dev;
			q->desc_count = vport->rxq_desc_count;
			q->vport = vport;
			q->rxq_grp = rx_qgrp;
			q->idx = (i * num_rxq) + j;
			q->rx_buf_size = IECM_RX_BUF_2048;
			q->rsc_low_watermark = IECM_LOW_WATERMARK;
			q->rx_max_pkt_size = vport->netdev->mtu +
					     IECM_PACKET_HDR_PAD;
			q->itr = rx_itr;
		}
	}
err_alloc:
	if (err)
		iecm_rxq_group_rel(vport);
	return err;
}

/**
 * iecm_vport_queue_grp_alloc_all - Allocate all queue groups/resources
 * @vport: vport with qgrps to allocate
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_vport_queue_grp_alloc_all(struct iecm_vport *vport)
{
	int num_txq, num_rxq;
	int err;

	iecm_vport_calc_numq_per_grp(vport, &num_txq, &num_rxq);

	err = iecm_txq_group_alloc(vport, num_txq);
	if (err)
		goto err_out;

	err = iecm_rxq_group_alloc(vport, num_rxq);
err_out:
	if (err)
		iecm_vport_queue_grp_rel_all(vport);
	return err;
}

/**
 * iecm_vport_queues_alloc - Allocate memory for all queues
 * @vport: virtual port
 *
 * Allocate memory for queues associated with a vport.	 Returns 0 on success,
 * negative on failure.
 */
int iecm_vport_queues_alloc(struct iecm_vport *vport)
{
	int err;

	err = iecm_vport_queue_grp_alloc_all(vport);
	if (err)
		goto err_out;

	err = iecm_tx_desc_alloc_all(vport);
	if (err)
		goto err_out;

	err = iecm_rx_desc_alloc_all(vport);
	if (err)
		goto err_out;

	err = iecm_vport_init_fast_path_txqs(vport);
	if (err)
		goto err_out;

	return 0;
err_out:
	iecm_vport_queues_rel(vport);
	return err;
}

/**
 * iecm_tx_find_q - Find the Tx q based on q id
 * @vport: the vport we care about
 * @q_id: Id of the queue
 *
 * Returns queue ptr if found else returns NULL
 */
static struct iecm_queue *
iecm_tx_find_q(struct iecm_vport *vport, int q_id)
{
	int i;

	for (i = 0; i < vport->num_txq; i++) {
		struct iecm_queue *tx_q = vport->txqs[i];

		if (tx_q->q_id == q_id)
			return tx_q;
	}

	return NULL;
}

/**
 * iecm_tx_handle_sw_marker - Handle queue marker packet
 * @tx_q: Tx queue to handle software marker
 */
static void iecm_tx_handle_sw_marker(struct iecm_queue *tx_q)
{
	struct iecm_vport *vport = tx_q->vport;
	bool drain_complete = true;
	int i;

	clear_bit(__IECM_Q_SW_MARKER, tx_q->flags);
	/* Hardware must write marker packets to all queues associated with
	 * completion queues. So check if all queues received marker packets
	 */
	for (i = 0; i < vport->num_txq; i++) {
		if (test_bit(__IECM_Q_SW_MARKER, vport->txqs[i]->flags))
			drain_complete = false;
	}
	if (drain_complete) {
		set_bit(__IECM_VPORT_SW_MARKER, vport->flags);
		wake_up(&vport->sw_marker_wq);
	}
}

/**
 * iecm_tx_splitq_clean_buf - Clean TX buffer resources
 * @tx_q: Tx queue to clean buffer from
 * @tx_buf: buffer to be cleaned
 * @napi_budget: Used to determine if we are in netpoll
 *
 * Returns the stats (bytes/packets) cleaned from this buffer
 */
static struct iecm_tx_queue_stats
iecm_tx_splitq_clean_buf(struct iecm_queue *tx_q, struct iecm_tx_buf *tx_buf,
			 int napi_budget)
{
	struct iecm_tx_queue_stats cleaned = {0};
	struct netdev_queue *nq;

	/* update the statistics for this packet */
	cleaned.bytes = tx_buf->bytecount;
	cleaned.packets = tx_buf->gso_segs;

	/* free the skb */
	napi_consume_skb(tx_buf->skb, napi_budget);
	nq = netdev_get_tx_queue(tx_q->vport->netdev, tx_q->idx);
	netdev_tx_completed_queue(nq, cleaned.packets,
				  cleaned.bytes);

	/* unmap skb header data */
	dma_unmap_single(tx_q->dev,
			 dma_unmap_addr(tx_buf, dma),
			 dma_unmap_len(tx_buf, len),
			 DMA_TO_DEVICE);

	/* clear tx_buf data */
	tx_buf->skb = NULL;
	dma_unmap_len_set(tx_buf, len, 0);

	return cleaned;
}

/**
 * iecm_stash_flow_sch_buffers - store buffere parameter info to be freed at a
 * later time (only relevant for flow scheduling mode)
 * @txq: Tx queue to clean
 * @tx_buf: buffer to store
 */
static int
iecm_stash_flow_sch_buffers(struct iecm_queue *txq, struct iecm_tx_buf *tx_buf)
{
	struct iecm_adapter *adapter = txq->vport->adapter;
	struct iecm_tx_buf *shadow_buf;

	shadow_buf = iecm_buf_lifo_pop(&txq->buf_stack);
	if (!shadow_buf) {
		dev_err(&adapter->pdev->dev,
			"No out-of-order TX buffers left!\n");
		return -ENOMEM;
	}

	/* Store buffer params in shadow buffer */
	shadow_buf->skb = tx_buf->skb;
	shadow_buf->bytecount = tx_buf->bytecount;
	shadow_buf->gso_segs = tx_buf->gso_segs;
	dma_unmap_addr_set(shadow_buf, dma, dma_unmap_addr(tx_buf, dma));
	dma_unmap_len_set(shadow_buf, len, dma_unmap_len(tx_buf, len));
	shadow_buf->compl_tag = tx_buf->compl_tag;

	/* Add buffer to buf_hash table to be freed
	 * later
	 */
	hash_add(txq->sched_buf_hash, &shadow_buf->hlist,
		 shadow_buf->compl_tag);

	memset(tx_buf, 0, sizeof(struct iecm_tx_buf));

	return 0;
}

/**
 * iecm_tx_splitq_clean - Reclaim resources from buffer queue
 * @tx_q: Tx queue to clean
 * @end: queue index until which it should be cleaned
 * @napi_budget: Used to determine if we are in netpoll
 * @descs_only: true if queue is using flow-based scheduling and should
 * not clean buffers at this time
 *
 * Cleans the queue descriptor ring. If the queue is using queue-based
 * scheduling, the buffers will be cleaned as well and this function will
 * return the number of bytes/packets cleaned. If the queue is using flow-based
 * scheduling, only the descriptors are cleaned at this time. Separate packet
 * completion events will be reported on the completion queue, and the
 * buffers will be cleaned separately. The stats returned from this function
 * when using flow-based scheduling are irrelevant.
 */
static struct iecm_tx_queue_stats
iecm_tx_splitq_clean(struct iecm_queue *tx_q, u16 end, int napi_budget,
		     bool descs_only)
{
	union iecm_tx_flex_desc *next_pending_desc = NULL;
	struct iecm_tx_queue_stats cleaned_stats = {0};
	union iecm_tx_flex_desc *tx_desc;
	s16 ntc = tx_q->next_to_clean;
	struct iecm_tx_buf *tx_buf;

	tx_desc = IECM_FLEX_TX_DESC(tx_q, ntc);
	next_pending_desc = IECM_FLEX_TX_DESC(tx_q, end);
	tx_buf = &tx_q->tx_buf[ntc];
	ntc -= tx_q->desc_count;

	while (tx_desc != next_pending_desc) {
		union iecm_tx_flex_desc *eop_desc =
			(union iecm_tx_flex_desc *)tx_buf->next_to_watch;

		/* clear next_to_watch to prevent false hangs */
		tx_buf->next_to_watch = NULL;

		if (descs_only) {
			if (iecm_stash_flow_sch_buffers(tx_q, tx_buf))
				goto tx_splitq_clean_out;

			while (tx_desc != eop_desc) {
				tx_buf++;
				tx_desc++;
				ntc++;
				if (unlikely(!ntc)) {
					ntc -= tx_q->desc_count;
					tx_buf = tx_q->tx_buf;
					tx_desc = IECM_FLEX_TX_DESC(tx_q, 0);
				}

				if (dma_unmap_len(tx_buf, len)) {
					if (iecm_stash_flow_sch_buffers(tx_q,
									tx_buf))
						goto tx_splitq_clean_out;
				}
			}
		} else {
			struct iecm_tx_queue_stats buf_stats = {0};

			buf_stats = iecm_tx_splitq_clean_buf(tx_q, tx_buf,
							     napi_budget);

			/* update the statistics for this packet */
			cleaned_stats.bytes += buf_stats.bytes;
			cleaned_stats.packets += buf_stats.packets;

			/* unmap remaining buffers */
			while (tx_desc != eop_desc) {
				tx_buf++;
				tx_desc++;
				ntc++;
				if (unlikely(!ntc)) {
					ntc -= tx_q->desc_count;
					tx_buf = tx_q->tx_buf;
					tx_desc = IECM_FLEX_TX_DESC(tx_q, 0);
				}

				/* unmap any remaining paged data */
				if (dma_unmap_len(tx_buf, len)) {
					dma_unmap_page(tx_q->dev,
						       dma_unmap_addr(tx_buf, dma),
						       dma_unmap_len(tx_buf, len),
						       DMA_TO_DEVICE);
					dma_unmap_len_set(tx_buf, len, 0);
				}
			}
		}

		tx_buf++;
		tx_desc++;
		ntc++;
		if (unlikely(!ntc)) {
			ntc -= tx_q->desc_count;
			tx_buf = tx_q->tx_buf;
			tx_desc = IECM_FLEX_TX_DESC(tx_q, 0);
		}
	}

tx_splitq_clean_out:
	ntc += tx_q->desc_count;
	tx_q->next_to_clean = ntc;

	return cleaned_stats;
}

/**
 * iecm_tx_clean_flow_sch_bufs - clean bufs that were stored for
 * out of order completions
 * @txq: queue to clean
 * @compl_tag: completion tag of packet to clean (from completion descriptor)
 * @desc_ts: pointer to 3 byte timestamp from descriptor
 * @budget: Used to determine if we are in netpoll
 */
static struct iecm_tx_queue_stats
iecm_tx_clean_flow_sch_bufs(struct iecm_queue *txq, u16 compl_tag,
			    u8 *desc_ts, int budget)
{
	struct iecm_tx_queue_stats cleaned_stats = {0};
	struct hlist_node *tmp_buf = NULL;
	struct iecm_tx_buf *tx_buf = NULL;

	/* Buffer completion */
	hash_for_each_possible_safe(txq->sched_buf_hash, tx_buf, tmp_buf,
				    hlist, compl_tag) {
		if (tx_buf->compl_tag != compl_tag)
			continue;

		if (likely(tx_buf->skb)) {
			cleaned_stats = iecm_tx_splitq_clean_buf(txq, tx_buf,
								 budget);
		} else if (dma_unmap_len(tx_buf, len)) {
			dma_unmap_page(txq->dev,
				       dma_unmap_addr(tx_buf, dma),
				       dma_unmap_len(tx_buf, len),
				       DMA_TO_DEVICE);
			dma_unmap_len_set(tx_buf, len, 0);
		}

		/* Push shadow buf back onto stack */
		iecm_buf_lifo_push(&txq->buf_stack, tx_buf);

		hash_del(&tx_buf->hlist);
	}

	return cleaned_stats;
}

/**
 * iecm_tx_clean_complq - Reclaim resources on completion queue
 * @complq: Tx ring to clean
 * @budget: Used to determine if we are in netpoll
 *
 * Returns true if there's any budget left (e.g. the clean is finished)
 */
static bool
iecm_tx_clean_complq(struct iecm_queue *complq, int budget)
{
	struct iecm_splitq_tx_compl_desc *tx_desc;
	struct iecm_vport *vport = complq->vport;
	s16 ntc = complq->next_to_clean;
	unsigned int complq_budget;

	complq_budget = vport->compln_clean_budget;
	tx_desc = IECM_SPLITQ_TX_COMPLQ_DESC(complq, ntc);
	ntc -= complq->desc_count;

	do {
		struct iecm_tx_queue_stats cleaned_stats = {0};
		bool descs_only = false;
		struct iecm_queue *tx_q;
		u16 compl_tag, hw_head;
		int tx_qid;
		u8 ctype;	/* completion type */
		u16 gen;

		/* if the descriptor isn't done, no work yet to do */
		gen = (le16_to_cpu(tx_desc->qid_comptype_gen) &
		      IECM_TXD_COMPLQ_GEN_M) >> IECM_TXD_COMPLQ_GEN_S;
		if (test_bit(__IECM_Q_GEN_CHK, complq->flags) != gen)
			break;

		/* Find necessary info of TX queue to clean buffers */
		tx_qid = (le16_to_cpu(tx_desc->qid_comptype_gen) &
			 IECM_TXD_COMPLQ_QID_M) >> IECM_TXD_COMPLQ_QID_S;
		tx_q = iecm_tx_find_q(vport, tx_qid);
		if (!tx_q) {
			dev_err(&complq->vport->adapter->pdev->dev,
				"TxQ #%d not found\n", tx_qid);
			goto fetch_next_desc;
		}

		/* Determine completion type */
		ctype = (le16_to_cpu(tx_desc->qid_comptype_gen) &
			IECM_TXD_COMPLQ_COMPL_TYPE_M) >>
			IECM_TXD_COMPLQ_COMPL_TYPE_S;
		switch (ctype) {
		case IECM_TXD_COMPLT_RE:
			hw_head = le16_to_cpu(tx_desc->q_head_compl_tag.q_head);

			cleaned_stats = iecm_tx_splitq_clean(tx_q, hw_head,
							     budget,
							     descs_only);
			break;
		case IECM_TXD_COMPLT_RS:
			if (test_bit(__IECM_Q_FLOW_SCH_EN, tx_q->flags)) {
				compl_tag =
				le16_to_cpu(tx_desc->q_head_compl_tag.compl_tag);

				cleaned_stats =
					iecm_tx_clean_flow_sch_bufs(tx_q,
								    compl_tag,
								    tx_desc->ts,
								    budget);
			} else {
				hw_head =
				le16_to_cpu(tx_desc->q_head_compl_tag.q_head);

				cleaned_stats = iecm_tx_splitq_clean(tx_q,
								     hw_head,
								     budget,
								     false);
			}

			break;
		case IECM_TXD_COMPLT_SW_MARKER:
			iecm_tx_handle_sw_marker(tx_q);
			break;
		default:
			dev_err(&tx_q->vport->adapter->pdev->dev,
				"Unknown TX completion type: %d\n",
				ctype);
			goto fetch_next_desc;
		}

		tx_q->itr.stats.tx.packets += cleaned_stats.packets;
		tx_q->itr.stats.tx.bytes += cleaned_stats.bytes;
		u64_stats_update_begin(&tx_q->stats_sync);
		tx_q->q_stats.tx.packets += cleaned_stats.packets;
		tx_q->q_stats.tx.bytes += cleaned_stats.bytes;
		u64_stats_update_end(&tx_q->stats_sync);

fetch_next_desc:
		tx_desc++;
		ntc++;
		if (unlikely(!ntc)) {
			ntc -= complq->desc_count;
			tx_desc = IECM_SPLITQ_TX_COMPLQ_DESC(complq, 0);
			change_bit(__IECM_Q_GEN_CHK, complq->flags);
		}

		prefetch(tx_desc);

		/* update budget accounting */
		complq_budget--;
	} while (likely(complq_budget));

	ntc += complq->desc_count;
	complq->next_to_clean = ntc;

	return !!complq_budget;
}

/**
 * iecm_tx_splitq_build_ctb - populate command tag and size for queue
 * based scheduling descriptors
 * @desc: descriptor to populate
 * @parms: pointer to Tx params struct
 * @td_cmd: command to be filled in desc
 * @size: size of buffer
 */
static void
iecm_tx_splitq_build_ctb(union iecm_tx_flex_desc *desc,
			 struct iecm_tx_splitq_params *parms,
			 u16 td_cmd, u16 size)
{
	desc->q.qw1.cmd_dtype =
		cpu_to_le16(parms->dtype & IECM_FLEX_TXD_QW1_DTYPE_M);
	desc->q.qw1.cmd_dtype |=
		cpu_to_le16((td_cmd << IECM_FLEX_TXD_QW1_CMD_S) &
			    IECM_FLEX_TXD_QW1_CMD_M);
	desc->q.qw1.buf_size = cpu_to_le16((u16)size);
}

/**
 * iecm_tx_splitq_build_flow_desc - populate command tag and size for flow
 * scheduling descriptors
 * @desc: descriptor to populate
 * @parms: pointer to Tx params struct
 * @td_cmd: command to be filled in desc
 * @size: size of buffer
 */
static void
iecm_tx_splitq_build_flow_desc(union iecm_tx_flex_desc *desc,
			       struct iecm_tx_splitq_params *parms,
			       u16 td_cmd, u16 size)
{
	desc->flow.qw1.cmd_dtype = (u16)parms->dtype | td_cmd;
	desc->flow.qw1.rxr_bufsize = cpu_to_le16((u16)size);
	desc->flow.qw1.compl_tag = cpu_to_le16(parms->compl_tag);

	desc->flow.qw1.ts[0] = parms->offload.desc_ts & 0xff;
	desc->flow.qw1.ts[1] = (parms->offload.desc_ts >> 8) & 0xff;
	desc->flow.qw1.ts[2] = (parms->offload.desc_ts >> 16) & 0xff;
}

/**
 * __iecm_tx_maybe_stop - 2nd level check for Tx stop conditions
 * @tx_q: the queue to be checked
 * @size: the size buffer we want to assure is available
 *
 * Returns -EBUSY if a stop is needed, else 0
 */
static int
__iecm_tx_maybe_stop(struct iecm_queue *tx_q, unsigned int size)
{
	netif_stop_subqueue(tx_q->vport->netdev, tx_q->idx);

	/* Memory barrier before checking head and tail */
	smp_mb();

	/* Check again in a case another CPU has just made room available. */
	if (likely(IECM_DESC_UNUSED(tx_q) < size))
		return -EBUSY;

	/* A reprieve! - use start_subqueue because it doesn't call schedule */
	netif_start_subqueue(tx_q->vport->netdev, tx_q->idx);

	return 0;
}

/**
 * iecm_tx_maybe_stop - 1st level check for Tx stop conditions
 * @tx_q: the queue to be checked
 * @size: number of descriptors we want to assure is available
 *
 * Returns 0 if stop is not needed
 */
int iecm_tx_maybe_stop(struct iecm_queue *tx_q, unsigned int size)
{
	if (likely(IECM_DESC_UNUSED(tx_q) >= size))
		return 0;

	return __iecm_tx_maybe_stop(tx_q, size);
}

/**
 * iecm_tx_buf_hw_update - Store the new tail and head values
 * @tx_q: queue to bump
 * @val: new head index
 */
void iecm_tx_buf_hw_update(struct iecm_queue *tx_q, u32 val)
{
	struct netdev_queue *nq;

	nq = netdev_get_tx_queue(tx_q->vport->netdev, tx_q->idx);
	tx_q->next_to_use = val;

	iecm_tx_maybe_stop(tx_q, IECM_TX_DESC_NEEDED);

	/* Force memory writes to complete before letting h/w
	 * know there are new descriptors to fetch.  (Only
	 * applicable for weak-ordered memory model archs,
	 * such as IA-64).
	 */
	wmb();

	/* notify HW of packet */
	if (netif_xmit_stopped(nq) || !netdev_xmit_more())
		writel(val, tx_q->tail);
}

/**
 * __iecm_tx_desc_count required - Get the number of descriptors needed for Tx
 * @size: transmit request size in bytes
 *
 * Due to hardware alignment restrictions (4K alignment), we need to
 * assume that we can have no more than 12K of data per descriptor, even
 * though each descriptor can take up to 16K - 1 bytes of aligned memory.
 * Thus, we need to divide by 12K. But division is slow! Instead,
 * we decompose the operation into shifts and one relatively cheap
 * multiply operation.
 *
 * To divide by 12K, we first divide by 4K, then divide by 3:
 *	To divide by 4K, shift right by 12 bits
 *	To divide by 3, multiply by 85, then divide by 256
 *	(Divide by 256 is done by shifting right by 8 bits)
 * Finally, we add one to round up. Because 256 isn't an exact multiple of
 * 3, we'll underestimate near each multiple of 12K. This is actually more
 * accurate as we have 4K - 1 of wiggle room that we can fit into the last
 * segment. For our purposes this is accurate out to 1M which is orders of
 * magnitude greater than our largest possible GSO size.
 *
 * This would then be implemented as:
 *	return (((size >> 12) * 85) >> 8) + IECM_TX_DESCS_FOR_SKB_DATA_PTR;
 *
 * Since multiplication and division are commutative, we can reorder
 * operations into:
 *	return ((size * 85) >> 20) + IECM_TX_DESCS_FOR_SKB_DATA_PTR;
 */
static unsigned int __iecm_tx_desc_count_required(unsigned int size)
{
	return ((size * 85) >> 20) + IECM_TX_DESCS_FOR_SKB_DATA_PTR;
}

/**
 * iecm_tx_desc_count_required - calculate number of Tx descriptors needed
 * @skb: send buffer
 *
 * Returns number of data descriptors needed for this skb.
 */
unsigned int iecm_tx_desc_count_required(struct sk_buff *skb)
{
	const skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags;
	unsigned int count = 0, size = skb_headlen(skb);

	for (;;) {
		count += __iecm_tx_desc_count_required(size);

		if (!nr_frags--)
			break;

		size = skb_frag_size(frag++);
	}

	return count;
}

/**
 * iecm_tx_splitq_map - Build the Tx flex descriptor
 * @tx_q: queue to send buffer on
 * @parms: pointer to splitq params struct
 * @first: first buffer info buffer to use
 *
 * This function loops over the skb data pointed to by *first
 * and gets a physical address for each memory location and programs
 * it and the length into the transmit flex descriptor.
 */
static void
iecm_tx_splitq_map(struct iecm_queue *tx_q,
		   struct iecm_tx_splitq_params *parms,
		   struct iecm_tx_buf *first)
{
	union iecm_tx_flex_desc *tx_desc;
	unsigned int data_len, size;
	struct iecm_tx_buf *tx_buf;
	u16 i = tx_q->next_to_use;
	struct netdev_queue *nq;
	struct sk_buff *skb;
	skb_frag_t *frag;
	u16 td_cmd = 0;
	dma_addr_t dma;

	skb = first->skb;

	td_cmd = parms->offload.td_cmd;
	parms->compl_tag = tx_q->tx_buf_key;

	data_len = skb->data_len;
	size = skb_headlen(skb);

	tx_desc = IECM_FLEX_TX_DESC(tx_q, i);

	dma = dma_map_single(tx_q->dev, skb->data, size, DMA_TO_DEVICE);

	tx_buf = first;

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		unsigned int max_data = IECM_TX_MAX_DESC_DATA_ALIGNED;

		if (dma_mapping_error(tx_q->dev, dma))
			goto dma_error;

		/* record length, and DMA address */
		dma_unmap_len_set(tx_buf, len, size);
		dma_unmap_addr_set(tx_buf, dma, dma);

		/* align size to end of page */
		max_data += -dma & (IECM_TX_MAX_READ_REQ_SIZE - 1);

		/* buf_addr is in same location for both desc types */
		tx_desc->q.buf_addr = cpu_to_le64(dma);

		/* account for data chunks larger than the hardware
		 * can handle
		 */
		while (unlikely(size > IECM_TX_MAX_DESC_DATA)) {
			parms->splitq_build_ctb(tx_desc, parms, td_cmd, size);

			tx_desc++;
			i++;

			if (i == tx_q->desc_count) {
				tx_desc = IECM_FLEX_TX_DESC(tx_q, 0);
				i = 0;
			}

			dma += max_data;
			size -= max_data;

			max_data = IECM_TX_MAX_DESC_DATA_ALIGNED;
			/* buf_addr is in same location for both desc types */
			tx_desc->q.buf_addr = cpu_to_le64(dma);
		}

		if (likely(!data_len))
			break;
		parms->splitq_build_ctb(tx_desc, parms, td_cmd, size);
		tx_desc++;
		i++;

		if (i == tx_q->desc_count) {
			tx_desc = IECM_FLEX_TX_DESC(tx_q, 0);
			i = 0;
		}

		size = skb_frag_size(frag);
		data_len -= size;

		dma = skb_frag_dma_map(tx_q->dev, frag, 0, size,
				       DMA_TO_DEVICE);

		tx_buf->compl_tag = parms->compl_tag;
		tx_buf = &tx_q->tx_buf[i];
	}

	/* record bytecount for BQL */
	nq = netdev_get_tx_queue(tx_q->vport->netdev, tx_q->idx);
	netdev_tx_sent_queue(nq, first->bytecount);

	/* record SW timestamp if HW timestamp is not available */
	skb_tx_timestamp(first->skb);

	/* write last descriptor with RS and EOP bits */
	td_cmd |= parms->eop_cmd;
	parms->splitq_build_ctb(tx_desc, parms, td_cmd, size);
	i++;
	if (i == tx_q->desc_count)
		i = 0;

	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;
	tx_buf->compl_tag = parms->compl_tag++;

	iecm_tx_buf_hw_update(tx_q, i);

	/* Update TXQ Completion Tag key for next buffer */
	tx_q->tx_buf_key = parms->compl_tag;

	return;

dma_error:
	/* clear DMA mappings for failed tx_buf map */
	for (;;) {
		tx_buf = &tx_q->tx_buf[i];
		iecm_tx_buf_rel(tx_q, tx_buf);
		if (tx_buf == first)
			break;
		if (i == 0)
			i = tx_q->desc_count;
		i--;
	}

	tx_q->next_to_use = i;
}

/**
 * iecm_tso - computes mss and TSO length to prepare for TSO
 * @first: pointer to struct iecm_tx_buf
 * @off: pointer to struct that holds offload parameters
 *
 * Returns error (negative) if TSO doesn't apply to the given skb,
 * 0 otherwise.
 *
 * Note: this function can be used in the splitq and singleq paths
 */
static int iecm_tso(struct iecm_tx_buf *first,
		    struct iecm_tx_offload_params *off)
{
	struct sk_buff *skb = first->skb;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		unsigned char *hdr;
	} l4;
	u32 paylen, l4_start;
	int err;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	if (!skb_is_gso(skb))
		return 0;

	err = skb_cow_head(skb, 0);
	if (err < 0)
		return err;

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);

	/* initialize outer IP header fields */
	if (ip.v4->version == 4) {
		ip.v4->tot_len = 0;
		ip.v4->check = 0;
	} else {
		ip.v6->payload_len = 0;
	}

	/* determine offset of transport header */
	l4_start = l4.hdr - skb->data;

	/* remove payload length from checksum */
	paylen = skb->len - l4_start;

	switch (skb_shinfo(skb)->gso_type) {
	case SKB_GSO_TCPV4:
	case SKB_GSO_TCPV6:
		csum_replace_by_diff(&l4.tcp->check,
				     (__force __wsum)htonl(paylen));

		/* compute length of segmentation header */
		off->tso_hdr_len = (l4.tcp->doff * 4) + l4_start;
		break;
	case SKB_GSO_UDP_L4:
		csum_replace_by_diff(&l4.udp->check,
				     (__force __wsum)htonl(paylen));
		/* compute length of segmentation header */
		off->tso_hdr_len = sizeof(struct udphdr) + l4_start;
		l4.udp->len =
			htons(skb_shinfo(skb)->gso_size +
			      sizeof(struct udphdr));
		break;
	default:
		return -EINVAL;
	}

	off->tso_len = skb->len - off->tso_hdr_len;
	off->mss = skb_shinfo(skb)->gso_size;

	/* update gso_segs and bytecount */
	first->gso_segs = skb_shinfo(skb)->gso_segs;
	first->bytecount += (first->gso_segs - 1) * off->tso_hdr_len;

	first->tx_flags |= IECM_TX_FLAGS_TSO;

	return 0;
}

/**
 * iecm_tx_splitq_frame - Sends buffer on Tx ring using flex descriptors
 * @skb: send buffer
 * @tx_q: queue to send buffer on
 *
 * Returns NETDEV_TX_OK if sent, else an error code
 */
static netdev_tx_t
iecm_tx_splitq_frame(struct sk_buff *skb, struct iecm_queue *tx_q)
{
	struct iecm_tx_splitq_params tx_parms = { NULL, 0, 0, 0, {0} };
	struct iecm_tx_buf *first;
	unsigned int count;

	count = iecm_tx_desc_count_required(skb);

	/* need: 1 descriptor per page * PAGE_SIZE/IECM_MAX_DATA_PER_TXD,
	 *       + 1 desc for skb_head_len/IECM_MAX_DATA_PER_TXD,
	 *       + 4 desc gap to avoid the cache line where head is,
	 *       + 1 desc for context descriptor,
	 * otherwise try next time
	 */
	if (iecm_tx_maybe_stop(tx_q, count + IECM_TX_DESCS_PER_CACHE_LINE +
			       IECM_TX_DESCS_FOR_CTX)) {
		return NETDEV_TX_BUSY;
	}

	/* record the location of the first descriptor for this packet */
	first = &tx_q->tx_buf[tx_q->next_to_use];
	first->skb = skb;
	first->bytecount = max_t(unsigned int, skb->len, ETH_ZLEN);
	first->gso_segs = 1;
	first->tx_flags = 0;

	if (iecm_tso(first, &tx_parms.offload) < 0) {
		/* If tso returns an error, drop the packet */
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	if (first->tx_flags & IECM_TX_FLAGS_TSO) {
		/* If TSO is needed, set up context desc */
		union iecm_flex_tx_ctx_desc *ctx_desc;
		int i = tx_q->next_to_use;

		/* grab the next descriptor */
		ctx_desc = IECM_FLEX_TX_CTX_DESC(tx_q, i);
		i++;
		tx_q->next_to_use = (i < tx_q->desc_count) ? i : 0;

		ctx_desc->tso.qw1.cmd_dtype |=
				cpu_to_le16(IECM_TX_DESC_DTYPE_FLEX_TSO_CTX |
					    IECM_TX_FLEX_CTX_DESC_CMD_TSO);
		ctx_desc->tso.qw0.flex_tlen =
				cpu_to_le32(tx_parms.offload.tso_len &
					    IECM_TXD_FLEX_CTX_TLEN_M);
		ctx_desc->tso.qw0.mss_rt =
				cpu_to_le16(tx_parms.offload.mss &
					    IECM_TXD_FLEX_CTX_MSS_RT_M);
		ctx_desc->tso.qw0.hdr_len = tx_parms.offload.tso_hdr_len;
	}

	if (test_bit(__IECM_Q_FLOW_SCH_EN, tx_q->flags)) {
		s64 ts_ns = first->skb->skb_mstamp_ns;

		tx_parms.offload.desc_ts =
			ts_ns >> IECM_TW_TIME_STAMP_GRAN_512_DIV_S;

		tx_parms.dtype = IECM_TX_DESC_DTYPE_FLEX_FLOW_SCHE;
		tx_parms.splitq_build_ctb = iecm_tx_splitq_build_flow_desc;
		tx_parms.eop_cmd =
			IECM_TXD_FLEX_FLOW_CMD_EOP | IECM_TXD_FLEX_FLOW_CMD_RE;

		if (skb->ip_summed == CHECKSUM_PARTIAL)
			tx_parms.offload.td_cmd |= IECM_TXD_FLEX_FLOW_CMD_CS_EN;

	} else {
		tx_parms.dtype = IECM_TX_DESC_DTYPE_FLEX_DATA;
		tx_parms.splitq_build_ctb = iecm_tx_splitq_build_ctb;
		tx_parms.eop_cmd = IECM_TX_DESC_CMD_EOP | IECM_TX_DESC_CMD_RS;

		if (skb->ip_summed == CHECKSUM_PARTIAL)
			tx_parms.offload.td_cmd |= IECM_TX_FLEX_DESC_CMD_CS_EN;
	}

	iecm_tx_splitq_map(tx_q, &tx_parms, first);

	return NETDEV_TX_OK;
}

/**
 * iecm_tx_splitq_start - Selects the right Tx queue to send buffer
 * @skb: send buffer
 * @netdev: network interface device structure
 *
 * Returns NETDEV_TX_OK if sent, else an error code
 */
netdev_tx_t iecm_tx_splitq_start(struct sk_buff *skb,
				 struct net_device *netdev)
{
	struct iecm_vport *vport = iecm_netdev_to_vport(netdev);
	struct iecm_queue *tx_q;

	tx_q = vport->txqs[skb->queue_mapping];

	/* hardware can't handle really short frames, hardware padding works
	 * beyond this point
	 */
	if (skb_put_padto(skb, IECM_TX_MIN_LEN))
		return NETDEV_TX_OK;

	return iecm_tx_splitq_frame(skb, tx_q);
}

/**
 * iecm_ptype_to_htype - get a hash type
 * @vport: virtual port data
 * @ptype: the ptype value from the descriptor
 *
 * Returns appropriate hash type (such as PKT_HASH_TYPE_L2/L3/L4) to be used by
 * skb_set_hash based on PTYPE as parsed by HW Rx pipeline and is part of
 * Rx desc.
 */
static enum pkt_hash_types iecm_ptype_to_htype(struct iecm_vport *vport,
					       u16 ptype)
{
	struct iecm_rx_ptype_decoded decoded = vport->rx_ptype_lkup[ptype];

	if (!decoded.known)
		return PKT_HASH_TYPE_NONE;
	if (decoded.payload_layer == IECM_RX_PTYPE_PAYLOAD_LAYER_PAY4)
		return PKT_HASH_TYPE_L4;
	if (decoded.payload_layer == IECM_RX_PTYPE_PAYLOAD_LAYER_PAY3)
		return PKT_HASH_TYPE_L3;
	if (decoded.outer_ip == IECM_RX_PTYPE_OUTER_L2)
		return PKT_HASH_TYPE_L2;

	return PKT_HASH_TYPE_NONE;
}

/**
 * iecm_rx_hash - set the hash value in the skb
 * @rxq: Rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being populated
 * @rx_desc: Receive descriptor
 * @ptype: the packet type decoded by hardware
 */
static void
iecm_rx_hash(struct iecm_queue *rxq, struct sk_buff *skb,
	     struct iecm_flex_rx_desc *rx_desc, u16 ptype)
{
	u32 hash;

	if (!iecm_is_feature_ena(rxq->vport, NETIF_F_RXHASH))
		return;

	hash = rx_desc->status_err1 |
	       (rx_desc->fflags1 << 8) |
	       (rx_desc->ts_low << 16) |
	       (rx_desc->ff2_mirrid_hash2.hash2 << 24);

	skb_set_hash(skb, hash, iecm_ptype_to_htype(rxq->vport, ptype));
}

/**
 * iecm_rx_csum - Indicate in skb if checksum is good
 * @rxq: Rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being populated
 * @rx_desc: Receive descriptor
 * @ptype: the packet type decoded by hardware
 *
 * skb->protocol must be set before this function is called
 */
static void
iecm_rx_csum(struct iecm_queue *rxq, struct sk_buff *skb,
	     struct iecm_flex_rx_desc *rx_desc, u16 ptype)
{
	struct iecm_rx_ptype_decoded decoded;
	u8 rx_status_0_qw1, rx_status_0_qw0;
	bool ipv4, ipv6;

	/* Start with CHECKSUM_NONE and by default csum_level = 0 */
	skb->ip_summed = CHECKSUM_NONE;

	/* check if Rx checksum is enabled */
	if (!iecm_is_feature_ena(rxq->vport, NETIF_F_RXCSUM))
		return;

	rx_status_0_qw1 = rx_desc->status_err0_qw1;
	/* check if HW has decoded the packet and checksum */
	if (!(rx_status_0_qw1 & BIT(IECM_RX_FLEX_DESC_STATUS0_L3L4P_S)))
		return;

	decoded = rxq->vport->rx_ptype_lkup[ptype];
	if (!(decoded.known && decoded.outer_ip))
		return;

	ipv4 = (decoded.outer_ip == IECM_RX_PTYPE_OUTER_IP) &&
	       (decoded.outer_ip_ver == IECM_RX_PTYPE_OUTER_IPV4);
	ipv6 = (decoded.outer_ip == IECM_RX_PTYPE_OUTER_IP) &&
	       (decoded.outer_ip_ver == IECM_RX_PTYPE_OUTER_IPV6);

	if (ipv4 && (rx_status_0_qw1 &
		     (BIT(IECM_RX_FLEX_DESC_STATUS0_XSUM_IPE_S) |
		      BIT(IECM_RX_FLEX_DESC_STATUS0_XSUM_EIPE_S))))
		goto checksum_fail;

	rx_status_0_qw0 = rx_desc->status_err0_qw0;
	if (ipv6 && (rx_status_0_qw0 &
		     (BIT(IECM_RX_FLEX_DESC_STATUS0_IPV6EXADD_S))))
		return;

	/* check for L4 errors and handle packets that were not able to be
	 * checksummed
	 */
	if (rx_status_0_qw1 & BIT(IECM_RX_FLEX_DESC_STATUS0_XSUM_L4E_S))
		goto checksum_fail;

	/* Only report checksum unnecessary for ICMP, TCP, UDP, or SCTP */
	switch (decoded.inner_prot) {
	case IECM_RX_PTYPE_INNER_PROT_ICMP:
	case IECM_RX_PTYPE_INNER_PROT_TCP:
	case IECM_RX_PTYPE_INNER_PROT_UDP:
	case IECM_RX_PTYPE_INNER_PROT_SCTP:
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		rxq->q_stats.rx.csum_unnecessary++;
	default:
		break;
	}
	return;

checksum_fail:
	rxq->q_stats.rx.csum_err++;
}

/**
 * iecm_rx_rsc - Set the RSC fields in the skb
 * @rxq : Rx descriptor ring packet is being transacted on
 * @skb : pointer to current skb being populated
 * @rx_desc: Receive descriptor
 * @ptype: the packet type decoded by hardware
 *
 * Populate the skb fields with the total number of RSC segments, RSC payload
 * length and packet type.
 */
static bool iecm_rx_rsc(struct iecm_queue *rxq, struct sk_buff *skb,
			struct iecm_flex_rx_desc *rx_desc, u16 ptype)
{
	struct iecm_rx_ptype_decoded decoded;
	u16 rsc_segments, rsc_payload_len;
	struct ipv6hdr *ipv6h;
	struct tcphdr *tcph;
	struct iphdr *ipv4h;
	bool ipv4, ipv6;
	u16 hdr_len;

	rsc_payload_len = le16_to_cpu(rx_desc->fmd1_misc.rscseglen);
	if (!rsc_payload_len)
		goto rsc_err;

	decoded = rxq->vport->rx_ptype_lkup[ptype];
	if (!(decoded.known && decoded.outer_ip))
		goto rsc_err;

	ipv4 = (decoded.outer_ip == IECM_RX_PTYPE_OUTER_IP) &&
		(decoded.outer_ip_ver == IECM_RX_PTYPE_OUTER_IPV4);
	ipv6 = (decoded.outer_ip == IECM_RX_PTYPE_OUTER_IP) &&
		(decoded.outer_ip_ver == IECM_RX_PTYPE_OUTER_IPV6);

	if (!(ipv4 ^ ipv6))
		goto rsc_err;

	if (ipv4)
		hdr_len = ETH_HLEN + sizeof(struct tcphdr) +
			  sizeof(struct iphdr);
	else
		hdr_len = ETH_HLEN + sizeof(struct tcphdr) +
			  sizeof(struct ipv6hdr);

	rsc_segments = DIV_ROUND_UP(skb->len - hdr_len, rsc_payload_len);

	NAPI_GRO_CB(skb)->count = rsc_segments;
	skb_shinfo(skb)->gso_size = rsc_payload_len;

	skb_reset_network_header(skb);

	if (ipv4) {
		ipv4h = ip_hdr(skb);
		skb_shinfo(skb)->gso_type = SKB_GSO_TCPV4;

		/* Reset and set transport header offset in skb */
		skb_set_transport_header(skb, sizeof(struct iphdr));
		tcph = tcp_hdr(skb);

		/* Compute the TCP pseudo header checksum*/
		tcph->check =
			~tcp_v4_check(skb->len - skb_transport_offset(skb),
				      ipv4h->saddr, ipv4h->daddr, 0);
	} else {
		ipv6h = ipv6_hdr(skb);
		skb_shinfo(skb)->gso_type = SKB_GSO_TCPV6;
		skb_set_transport_header(skb, sizeof(struct ipv6hdr));
		tcph = tcp_hdr(skb);
		tcph->check =
			~tcp_v6_check(skb->len - skb_transport_offset(skb),
				      &ipv6h->saddr, &ipv6h->daddr, 0);
	}

	tcp_gro_complete(skb);

	/* Map Rx qid to the skb*/
	skb_record_rx_queue(skb, rxq->q_id);

	return true;
rsc_err:
	return false;
}

/**
 * iecm_rx_process_skb_fields - Populate skb header fields from Rx descriptor
 * @rxq: Rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being populated
 * @rx_desc: Receive descriptor
 *
 * This function checks the ring, descriptor, and packet information in
 * order to populate the hash, checksum, VLAN, protocol, and
 * other fields within the skb.
 */
static bool
iecm_rx_process_skb_fields(struct iecm_queue *rxq, struct sk_buff *skb,
			   struct iecm_flex_rx_desc *rx_desc)
{
	bool err = false;
	u16 rx_ptype;
	bool rsc;

	rx_ptype = le16_to_cpu(rx_desc->ptype_err_fflags0) &
		   IECM_RXD_FLEX_PTYPE_M;

	/* modifies the skb - consumes the enet header */
	skb->protocol = eth_type_trans(skb, rxq->vport->netdev);
	iecm_rx_csum(rxq, skb, rx_desc, rx_ptype);
	/* process RSS/hash */
	iecm_rx_hash(rxq, skb, rx_desc, rx_ptype);

	rsc = le16_to_cpu(rx_desc->hdrlen_flags) & IECM_RXD_FLEX_RSC_M;
	if (rsc)
		err = iecm_rx_rsc(rxq, skb, rx_desc, rx_ptype);

	return err;
}

/**
 * iecm_rx_skb - Send a completed packet up the stack
 * @rxq: Rx ring in play
 * @skb: packet to send up
 *
 * This function sends the completed packet (via. skb) up the stack using
 * GRO receive functions
 */
void iecm_rx_skb(struct iecm_queue *rxq, struct sk_buff *skb)
{
	napi_gro_receive(&rxq->q_vector->napi, skb);
}

/**
 * iecm_rx_page_is_reserved - check if reuse is possible
 * @page: page struct to check
 */
static bool iecm_rx_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

/**
 * iecm_rx_buf_adjust_pg_offset - Prepare Rx buffer for reuse
 * @rx_buf: Rx buffer to adjust
 * @size: Size of adjustment
 *
 * Update the offset within page so that Rx buf will be ready to be reused.
 * For systems with PAGE_SIZE < 8192 this function will flip the page offset
 * so the second half of page assigned to Rx buffer will be used, otherwise
 * the offset is moved by the @size bytes
 */
static void
iecm_rx_buf_adjust_pg_offset(struct iecm_rx_buf *rx_buf, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	/* flip page offset to other buffer */
	rx_buf->page_offset ^= size;
#else
	/* move offset up to the next cache line */
	rx_buf->page_offset += size;
#endif
}

/**
 * iecm_rx_can_reuse_page - Determine if page can be reused for another Rx
 * @rx_buf: buffer containing the page
 *
 * If page is reusable, we have a green light for calling iecm_reuse_rx_page,
 * which will assign the current buffer to the buffer that next_to_alloc is
 * pointing to; otherwise, the DMA mapping needs to be destroyed and
 * page freed
 */
static bool iecm_rx_can_reuse_page(struct iecm_rx_buf *rx_buf)
{
#if (PAGE_SIZE >= 8192)
	unsigned int last_offset = PAGE_SIZE - IECM_RX_BUF_2048;
#endif
	unsigned int pagecnt_bias = rx_buf->pagecnt_bias;
	struct page *page = rx_buf->page;

	/* avoid re-using remote pages */
	if (unlikely(iecm_rx_page_is_reserved(page)))
		return false;

#if (PAGE_SIZE < 8192)
	/* if we are only owner of page we can reuse it */
	if (unlikely((page_count(page) - pagecnt_bias) > 1))
		return false;
#else
	if (rx_buf->page_offset > last_offset)
		return false;
#endif /* PAGE_SIZE < 8192) */

	/* If we have drained the page fragment pool we need to update
	 * the pagecnt_bias and page count so that we fully restock the
	 * number of references the driver holds.
	 */
	if (unlikely(pagecnt_bias == 1)) {
		page_ref_add(page, USHRT_MAX - 1);
		rx_buf->pagecnt_bias = USHRT_MAX;
	}

	return true;
}

/**
 * iecm_rx_add_frag - Add contents of Rx buffer to sk_buff as a frag
 * @rx_buf: buffer containing page to add
 * @skb: sk_buff to place the data into
 * @size: packet length from rx_desc
 *
 * This function will add the data contained in rx_buf->page to the skb.
 * It will just attach the page as a frag to the skb.
 * The function will then update the page offset.
 */
void iecm_rx_add_frag(struct iecm_rx_buf *rx_buf, struct sk_buff *skb,
		      unsigned int size)
{
#if (PAGE_SIZE >= 8192)
	unsigned int truesize = SKB_DATA_ALIGN(size);
#else
	unsigned int truesize = IECM_RX_BUF_2048;
#endif

	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, rx_buf->page,
			rx_buf->page_offset, size, truesize);

	/* page is being used so we must update the page offset */
	iecm_rx_buf_adjust_pg_offset(rx_buf, truesize);
}

/**
 * iecm_rx_reuse_page - page flip buffer and store it back on the queue
 * @rx_bufq: Rx descriptor ring to store buffers on
 * @hsplit: true if header buffer, false otherwise
 * @old_buf: donor buffer to have page reused
 *
 * Synchronizes page for reuse by the adapter
 */
void iecm_rx_reuse_page(struct iecm_queue *rx_bufq,
			bool hsplit,
			struct iecm_rx_buf *old_buf)
{
	u16 ntu = rx_bufq->next_to_use;
	struct iecm_rx_buf *new_buf;

	if (hsplit)
		new_buf = &rx_bufq->rx_buf.hdr_buf[ntu];
	else
		new_buf = &rx_bufq->rx_buf.buf[ntu];

	/* Transfer page from old buffer to new buffer.
	 * Move each member individually to avoid possible store
	 * forwarding stalls and unnecessary copy of skb.
	 */
	new_buf->dma = old_buf->dma;
	new_buf->page = old_buf->page;
	new_buf->page_offset = old_buf->page_offset;
	new_buf->pagecnt_bias = old_buf->pagecnt_bias;
}

/**
 * iecm_rx_get_buf_page - Fetch Rx buffer page and synchronize data for use
 * @dev: device struct
 * @rx_buf: Rx buf to fetch page for
 * @size: size of buffer to add to skb
 * @dev: device struct
 *
 * This function will pull an Rx buffer page from the ring and synchronize it
 * for use by the CPU.
 */
static void
iecm_rx_get_buf_page(struct device *dev, struct iecm_rx_buf *rx_buf,
		     const unsigned int size)
{
	prefetch(rx_buf->page);

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(dev, rx_buf->dma,
				      rx_buf->page_offset, size,
				      DMA_FROM_DEVICE);

	/* We have pulled a buffer for use, so decrement pagecnt_bias */
	rx_buf->pagecnt_bias--;
}

/**
 * iecm_rx_construct_skb - Allocate skb and populate it
 * @rxq: Rx descriptor queue
 * @rx_buf: Rx buffer to pull data from
 * @size: the length of the packet
 *
 * This function allocates an skb. It then populates it with the page
 * data from the current receive descriptor, taking care to set up the
 * skb correctly.
 */
struct sk_buff *
iecm_rx_construct_skb(struct iecm_queue *rxq, struct iecm_rx_buf *rx_buf,
		      unsigned int size)
{
	void *va = page_address(rx_buf->page) + rx_buf->page_offset;
	unsigned int headlen;
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch((u8 *)va + L1_CACHE_BYTES);
#endif /* L1_CACHE_BYTES */
	/* allocate a skb to store the frags */
	skb = __napi_alloc_skb(&rxq->q_vector->napi, IECM_RX_HDR_SIZE,
			       GFP_ATOMIC | __GFP_NOWARN);
	if (unlikely(!skb))
		return NULL;

	skb_record_rx_queue(skb, rxq->idx);

	/* Determine available headroom for copy */
	headlen = size;
	if (headlen > IECM_RX_HDR_SIZE)
		headlen = eth_get_headlen(skb->dev, va, IECM_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	memcpy(__skb_put(skb, headlen), va, ALIGN(headlen, sizeof(long)));

	/* if we exhaust the linear part then add what is left as a frag */
	size -= headlen;
	if (size) {
#if (PAGE_SIZE >= 8192)
		unsigned int truesize = SKB_DATA_ALIGN(size);
#else
		unsigned int truesize = IECM_RX_BUF_2048;
#endif
		skb_add_rx_frag(skb, 0, rx_buf->page,
				rx_buf->page_offset + headlen, size, truesize);
		/* buffer is used by skb, update page_offset */
		iecm_rx_buf_adjust_pg_offset(rx_buf, truesize);
	} else {
		/* buffer is unused, reset bias back to rx_buf; data was copied
		 * onto skb's linear part so there's no need for adjusting
		 * page offset and we can reuse this buffer as-is
		 */
		rx_buf->pagecnt_bias++;
	}

	return skb;
}

/**
 * iecm_rx_cleanup_headers - Correct empty headers
 * @skb: pointer to current skb being fixed
 *
 * Also address the case where we are pulling data in on pages only
 * and as such no data is present in the skb header.
 *
 * In addition if skb is not at least 60 bytes we need to pad it so that
 * it is large enough to qualify as a valid Ethernet frame.
 *
 * Returns true if an error was encountered and skb was freed.
 */
bool iecm_rx_cleanup_headers(struct sk_buff *skb)
{
	/* if eth_skb_pad returns an error the skb was freed */
	if (eth_skb_pad(skb))
		return true;

	return false;
}

/**
 * iecm_rx_splitq_test_staterr - tests bits in Rx descriptor
 * status and error fields
 * @stat_err_field: field from descriptor to test bits in
 * @stat_err_bits: value to mask
 *
 */
static bool
iecm_rx_splitq_test_staterr(u8 stat_err_field, const u8 stat_err_bits)
{
	return !!(stat_err_field & stat_err_bits);
}

/**
 * iecm_rx_splitq_is_non_eop - process handling of non-EOP buffers
 * @rx_desc: Rx descriptor for current buffer
 *
 * If the buffer is an EOP buffer, this function exits returning false,
 * otherwise return true indicating that this is in fact a non-EOP buffer.
 */
static bool
iecm_rx_splitq_is_non_eop(struct iecm_flex_rx_desc *rx_desc)
{
	/* if we are the last buffer then there is nothing else to do */
#define IECM_RXD_EOF BIT(IECM_RX_FLEX_DESC_STATUS0_EOF_S)
	if (likely(iecm_rx_splitq_test_staterr(rx_desc->status_err0_qw1,
					       IECM_RXD_EOF)))
		return false;

	return true;
}

/**
 * iecm_rx_recycle_buf - Clean up used buffer and either recycle or free
 * @rx_bufq: Rx descriptor queue to transact packets on
 * @hsplit: true if buffer is a header buffer
 * @rx_buf: Rx buffer to pull data from
 *
 * This function will clean up the contents of the rx_buf. It will either
 * recycle the buffer or unmap it and free the associated resources.
 *
 * Returns true if the buffer is reused, false if the buffer is freed.
 */
bool iecm_rx_recycle_buf(struct iecm_queue *rx_bufq, bool hsplit,
			 struct iecm_rx_buf *rx_buf)
{
	bool recycled = false;

	if (iecm_rx_can_reuse_page(rx_buf)) {
		/* hand second half of page back to the queue */
		iecm_rx_reuse_page(rx_bufq, hsplit, rx_buf);
		recycled = true;
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_bufq->dev, rx_buf->dma, PAGE_SIZE,
				     DMA_FROM_DEVICE, IECM_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buf->page, rx_buf->pagecnt_bias);
	}

	/* clear contents of buffer_info */
	rx_buf->page = NULL;
	rx_buf->skb = NULL;

	return recycled;
}

/**
 * iecm_rx_splitq_put_bufs - wrapper function to clean and recycle buffers
 * @rx_bufq: Rx descriptor queue to transact packets on
 * @hdr_buf: Rx header buffer to pull data from
 * @rx_buf: Rx buffer to pull data from
 *
 * This function will update the next_to_use/next_to_alloc if the current
 * buffer is recycled.
 */
static void iecm_rx_splitq_put_bufs(struct iecm_queue *rx_bufq,
				    struct iecm_rx_buf *hdr_buf,
				    struct iecm_rx_buf *rx_buf)
{
	u16 ntu = rx_bufq->next_to_use;
	bool recycled = false;

	if (likely(hdr_buf))
		recycled = iecm_rx_recycle_buf(rx_bufq, true, hdr_buf);
	if (likely(rx_buf))
		recycled = iecm_rx_recycle_buf(rx_bufq, false, rx_buf);

	/* update, and store next to alloc if the buffer was recycled */
	if (recycled) {
		ntu++;
		rx_bufq->next_to_use = (ntu < rx_bufq->desc_count) ? ntu : 0;
	}
}

/**
 * iecm_rx_bump_ntc - Bump and wrap q->next_to_clean value
 * @q: queue to bump
 */
static void iecm_rx_bump_ntc(struct iecm_queue *q)
{
	u16 ntc = q->next_to_clean + 1;
	/* fetch, update, and store next to clean */
	if (ntc < q->desc_count) {
		q->next_to_clean = ntc;
	} else {
		q->next_to_clean = 0;
		change_bit(__IECM_Q_GEN_CHK, q->flags);
	}
}

/**
 * iecm_rx_splitq_clean - Clean completed descriptors from Rx queue
 * @rxq: Rx descriptor queue to retrieve receive buffer queue
 * @budget: Total limit on number of packets to process
 *
 * This function provides a "bounce buffer" approach to Rx interrupt
 * processing. The advantage to this is that on systems that have
 * expensive overhead for IOMMU access this provides a means of avoiding
 * it by maintaining the mapping of the page to the system.
 *
 * Returns amount of work completed
 */
static int iecm_rx_splitq_clean(struct iecm_queue *rxq, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_pkts = 0;
	u16 cleaned_count[IECM_BUFQS_PER_RXQ_SET] = {0};
	struct iecm_queue *rx_bufq = NULL;
	struct sk_buff *skb = rxq->skb;
	bool failure = false;
	int i;

	/* Process Rx packets bounded by budget */
	while (likely(total_rx_pkts < (unsigned int)budget)) {
		struct iecm_flex_rx_desc *splitq_flex_rx_desc;
		union iecm_rx_desc *rx_desc;
		struct iecm_rx_buf *hdr_buf = NULL;
		struct iecm_rx_buf *rx_buf = NULL;
		unsigned int pkt_len = 0;
		unsigned int hdr_len = 0;
		u16 gen_id, buf_id;
		u8 stat_err0_qw0;
		u8 stat_err_bits;
		 /* Header buffer overflow only valid for header split */
		bool hbo = false;
		int bufq_id;

		/* get the Rx desc from Rx queue based on 'next_to_clean' */
		rx_desc = IECM_RX_DESC(rxq, rxq->next_to_clean);
		splitq_flex_rx_desc = (struct iecm_flex_rx_desc *)rx_desc;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc
		 */
		dma_rmb();

		/* if the descriptor isn't done, no work yet to do */
		gen_id = le16_to_cpu(splitq_flex_rx_desc->pktlen_gen_bufq_id);
		gen_id = (gen_id & IECM_RXD_FLEX_GEN_M) >> IECM_RXD_FLEX_GEN_S;
		if (test_bit(__IECM_Q_GEN_CHK, rxq->flags) != gen_id)
			break;

		pkt_len = le16_to_cpu(splitq_flex_rx_desc->pktlen_gen_bufq_id) &
			  IECM_RXD_FLEX_LEN_PBUF_M;

		hbo = splitq_flex_rx_desc->status_err0_qw1 &
		      BIT(IECM_RX_FLEX_DESC_STATUS0_HBO_S);

		if (unlikely(hbo)) {
			rxq->q_stats.rx.hsplit_hbo++;
			goto bypass_hsplit;
		}

		hdr_len =
			le16_to_cpu(splitq_flex_rx_desc->hdrlen_flags) &
			IECM_RXD_FLEX_LEN_HDR_M;

bypass_hsplit:
		bufq_id = le16_to_cpu(splitq_flex_rx_desc->pktlen_gen_bufq_id);
		bufq_id = (bufq_id & IECM_RXD_FLEX_BUFQ_ID_M) >>
			  IECM_RXD_FLEX_BUFQ_ID_S;
		/* retrieve buffer from the rxq */
		rx_bufq = &rxq->rxq_grp->splitq.bufq_sets[bufq_id].bufq;

		buf_id = le16_to_cpu(splitq_flex_rx_desc->fmd0_bufid.buf_id);

		if (pkt_len) {
			rx_buf = &rx_bufq->rx_buf.buf[buf_id];
			iecm_rx_get_buf_page(rx_bufq->dev, rx_buf, pkt_len);
		}

		if (hdr_len) {
			hdr_buf = &rx_bufq->rx_buf.hdr_buf[buf_id];
			iecm_rx_get_buf_page(rx_bufq->dev, hdr_buf,
					     hdr_len);

			skb = iecm_rx_construct_skb(rxq, hdr_buf, hdr_len);
		}

		if (skb && pkt_len)
			iecm_rx_add_frag(rx_buf, skb, pkt_len);
		else if (pkt_len)
			skb = iecm_rx_construct_skb(rxq, rx_buf, pkt_len);

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			/* If we fetched a buffer, but didn't use it
			 * undo pagecnt_bias decrement
			 */
			if (rx_buf)
				rx_buf->pagecnt_bias++;
			break;
		}

		iecm_rx_splitq_put_bufs(rx_bufq, hdr_buf, rx_buf);
		iecm_rx_bump_ntc(rxq);
		cleaned_count[bufq_id]++;

		/* skip if it is non EOP desc */
		if (iecm_rx_splitq_is_non_eop(splitq_flex_rx_desc))
			continue;

		stat_err_bits = BIT(IECM_RX_FLEX_DESC_STATUS0_RXE_S);
		stat_err0_qw0 = splitq_flex_rx_desc->status_err0_qw0;
		if (unlikely(iecm_rx_splitq_test_staterr(stat_err0_qw0,
							 stat_err_bits))) {
			dev_kfree_skb_any(skb);
			skb = NULL;
			continue;
		}

		/* correct empty headers and pad skb if needed (to make valid
		 * Ethernet frame
		 */
		if (iecm_rx_cleanup_headers(skb)) {
			skb = NULL;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* protocol */
		if (unlikely(iecm_rx_process_skb_fields(rxq, skb,
							splitq_flex_rx_desc))) {
			dev_kfree_skb_any(skb);
			skb = NULL;
			continue;
		}

		/* send completed skb up the stack */
		iecm_rx_skb(rxq, skb);
		skb = NULL;

		/* update budget accounting */
		total_rx_pkts++;
	}
	for (i = 0; i < IECM_BUFQS_PER_RXQ_SET; i++) {
		if (cleaned_count[i]) {
			rx_bufq = &rxq->rxq_grp->splitq.bufq_sets[i].bufq;
			failure = iecm_rx_buf_hw_alloc_all(rx_bufq,
							   cleaned_count[i]) ||
				  failure;
		}
	}

	rxq->skb = skb;
	u64_stats_update_begin(&rxq->stats_sync);
	rxq->q_stats.rx.packets += total_rx_pkts;
	rxq->q_stats.rx.bytes += total_rx_bytes;
	u64_stats_update_end(&rxq->stats_sync);

	rxq->itr.stats.rx.packets += total_rx_pkts;
	rxq->itr.stats.rx.bytes += total_rx_bytes;

	/* guarantee a trip back through this routine if there was a failure */
	return failure ? budget : (int)total_rx_pkts;
}

/**
 * iecm_vport_intr_clean_queues - MSIX mode Interrupt Handler
 * @irq: interrupt number
 * @data: pointer to a q_vector
 *
 */
irqreturn_t
iecm_vport_intr_clean_queues(int __always_unused irq, void *data)
{
	struct iecm_q_vector *q_vector = (struct iecm_q_vector *)data;

	napi_schedule(&q_vector->napi);

	return IRQ_HANDLED;
}

/**
 * iecm_vport_intr_napi_dis_all - Disable NAPI for all q_vectors in the vport
 * @vport: main vport structure
 */
static void iecm_vport_intr_napi_dis_all(struct iecm_vport *vport)
{
	int q_idx;

	if (!vport->netdev)
		return;

	for (q_idx = 0; q_idx < vport->num_q_vectors; q_idx++) {
		struct iecm_q_vector *q_vector = &vport->q_vectors[q_idx];

		napi_disable(&q_vector->napi);
	}
}

/**
 * iecm_vport_intr_rel - Free memory allocated for interrupt vectors
 * @vport: virtual port
 *
 * Free the memory allocated for interrupt vectors  associated to a vport
 */
void iecm_vport_intr_rel(struct iecm_vport *vport)
{
	int i, j, v_idx;

	if (!vport->netdev)
		return;

	for (v_idx = 0; v_idx < vport->num_q_vectors; v_idx++) {
		struct iecm_q_vector *q_vector = &vport->q_vectors[v_idx];

		if (q_vector)
			netif_napi_del(&q_vector->napi);
	}

	/* Clean up the mapping of queues to vectors */
	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];

		if (iecm_is_queue_model_split(vport->rxq_model)) {
			for (j = 0; j < rx_qgrp->splitq.num_rxq_sets; j++)
				rx_qgrp->splitq.rxq_sets[j].rxq.q_vector =
									   NULL;
		} else {
			for (j = 0; j < rx_qgrp->singleq.num_rxq; j++)
				rx_qgrp->singleq.rxqs[j].q_vector = NULL;
		}
	}

	if (iecm_is_queue_model_split(vport->txq_model)) {
		for (i = 0; i < vport->num_txq_grp; i++)
			vport->txq_grps[i].complq->q_vector = NULL;
	} else {
		for (i = 0; i < vport->num_txq_grp; i++) {
			for (j = 0; j < vport->txq_grps[i].num_txq; j++)
				vport->txq_grps[i].txqs[j].q_vector = NULL;
		}
	}

	kfree(vport->q_vectors);
	vport->q_vectors = NULL;
}

/**
 * iecm_vport_intr_rel_irq - Free the IRQ association with the OS
 * @vport: main vport structure
 */
static void iecm_vport_intr_rel_irq(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	int vector;

	for (vector = 0; vector < vport->num_q_vectors; vector++) {
		struct iecm_q_vector *q_vector = &vport->q_vectors[vector];
		int irq_num, vidx;

		/* free only the IRQs that were actually requested */
		if (!q_vector)
			continue;

		vidx = vector + vport->q_vector_base;
		irq_num = adapter->msix_entries[vidx].vector;

		/* clear the affinity_mask in the IRQ descriptor */
		irq_set_affinity_hint(irq_num, NULL);
		free_irq(irq_num, q_vector);
	}
}

/**
 * iecm_vport_intr_dis_irq_all - Disable each interrupt
 * @vport: main vport structure
 */
void iecm_vport_intr_dis_irq_all(struct iecm_vport *vport)
{
	struct iecm_q_vector *q_vector = vport->q_vectors;
	struct iecm_hw *hw = &vport->adapter->hw;
	int q_idx;

	for (q_idx = 0; q_idx < vport->num_q_vectors; q_idx++)
		wr32(hw, q_vector[q_idx].intr_reg.dyn_ctl, 0);
}

/**
 * iecm_vport_intr_buildreg_itr - Enable default interrupt generation settings
 * @q_vector: pointer to q_vector
 * @type: ITR index
 * @itr: ITR value
 */
static u32 iecm_vport_intr_buildreg_itr(struct iecm_q_vector *q_vector,
					const int type, u16 itr)
{
	u32 itr_val;

	itr &= IECM_ITR_MASK;
	/* Don't clear PBA because that can cause lost interrupts that
	 * came in while we were cleaning/polling
	 */
	itr_val = q_vector->intr_reg.dyn_ctl_intena_m |
		  (type << q_vector->intr_reg.dyn_ctl_itridx_s) |
		  (itr << (q_vector->intr_reg.dyn_ctl_intrvl_s - 1));

	return itr_val;
}

static unsigned int iecm_itr_divisor(struct iecm_q_vector *q_vector)
{
	unsigned int divisor;

	switch (q_vector->vport->adapter->link_speed) {
	case VIRTCHNL_LINK_SPEED_40GB:
		divisor = IECM_ITR_ADAPTIVE_MIN_INC * 1024;
		break;
	case VIRTCHNL_LINK_SPEED_25GB:
	case VIRTCHNL_LINK_SPEED_20GB:
		divisor = IECM_ITR_ADAPTIVE_MIN_INC * 512;
		break;
	default:
	case VIRTCHNL_LINK_SPEED_10GB:
		divisor = IECM_ITR_ADAPTIVE_MIN_INC * 256;
		break;
	case VIRTCHNL_LINK_SPEED_1GB:
	case VIRTCHNL_LINK_SPEED_100MB:
		divisor = IECM_ITR_ADAPTIVE_MIN_INC * 32;
		break;
	}

	return divisor;
}

/**
 * iecm_vport_intr_set_new_itr - update the ITR value based on statistics
 * @q_vector: structure containing interrupt and ring information
 * @itr: structure containing queue performance data
 * @q_type: queue type
 *
 * Stores a new ITR value based on packets and byte
 * counts during the last interrupt.  The advantage of per interrupt
 * computation is faster updates and more accurate ITR for the current
 * traffic pattern.  Constants in this function were computed
 * based on theoretical maximum wire speed and thresholds were set based
 * on testing data as well as attempting to minimize response time
 * while increasing bulk throughput.
 */
static void iecm_vport_intr_set_new_itr(struct iecm_q_vector *q_vector,
					struct iecm_itr *itr,
					enum virtchnl_queue_type q_type)
{
	unsigned int avg_wire_size, packets = 0, bytes = 0, new_itr;
	unsigned long next_update = jiffies;

	/* If we don't have any queues just leave ourselves set for maximum
	 * possible latency so we take ourselves out of the equation.
	 */
	if (!IECM_ITR_IS_DYNAMIC(itr->target_itr))
		return;

	/* For Rx we want to push the delay up and default to low latency.
	 * for Tx we want to pull the delay down and default to high latency.
	 */
	new_itr = q_type == VIRTCHNL_QUEUE_TYPE_RX ?
	      IECM_ITR_ADAPTIVE_MIN_USECS | IECM_ITR_ADAPTIVE_LATENCY :
	      IECM_ITR_ADAPTIVE_MAX_USECS | IECM_ITR_ADAPTIVE_LATENCY;

	/* If we didn't update within up to 1 - 2 jiffies we can assume
	 * that either packets are coming in so slow there hasn't been
	 * any work, or that there is so much work that NAPI is dealing
	 * with interrupt moderation and we don't need to do anything.
	 */
	if (time_after(next_update, itr->next_update))
		goto clear_counts;

	/* If itr_countdown is set it means we programmed an ITR within
	 * the last 4 interrupt cycles. This has a side effect of us
	 * potentially firing an early interrupt. In order to work around
	 * this we need to throw out any data received for a few
	 * interrupts following the update.
	 */
	if (q_vector->itr_countdown) {
		new_itr = itr->target_itr;
		goto clear_counts;
	}

	if (q_type == VIRTCHNL_QUEUE_TYPE_TX) {
		packets = itr->stats.tx.packets;
		bytes = itr->stats.tx.bytes;
	}

	if (q_type == VIRTCHNL_QUEUE_TYPE_RX) {
		packets = itr->stats.rx.packets;
		bytes = itr->stats.rx.bytes;

		/* If there are 1 to 4 RX packets and bytes are less than
		 * 9000 assume insufficient data to use bulk rate limiting
		 * approach unless Tx is already in bulk rate limiting. We
		 * are likely latency driven.
		 */
		if (packets && packets < 4 && bytes < 9000 &&
		    (q_vector->tx[0]->itr.target_itr &
		     IECM_ITR_ADAPTIVE_LATENCY)) {
			new_itr = IECM_ITR_ADAPTIVE_LATENCY;
			goto adjust_by_size;
		}
	} else if (packets < 4) {
		/* If we have Tx and Rx ITR maxed and Tx ITR is running in
		 * bulk mode and we are receiving 4 or fewer packets just
		 * reset the ITR_ADAPTIVE_LATENCY bit for latency mode so
		 * that the Rx can relax.
		 */
		if (itr->target_itr == IECM_ITR_ADAPTIVE_MAX_USECS &&
		    ((q_vector->rx[0]->itr.target_itr & IECM_ITR_MASK) ==
		     IECM_ITR_ADAPTIVE_MAX_USECS))
			goto clear_counts;
	} else if (packets > 32) {
		/* If we have processed over 32 packets in a single interrupt
		 * for Tx assume we need to switch over to "bulk" mode.
		 */
		itr->target_itr &= ~IECM_ITR_ADAPTIVE_LATENCY;
	}

	/* We have no packets to actually measure against. This means
	 * either one of the other queues on this vector is active or
	 * we are a Tx queue doing TSO with too high of an interrupt rate.
	 *
	 * Between 4 and 56 we can assume that our current interrupt delay
	 * is only slightly too low. As such we should increase it by a small
	 * fixed amount.
	 */
	if (packets < 56) {
		new_itr = itr->target_itr + IECM_ITR_ADAPTIVE_MIN_INC;
		if ((new_itr & IECM_ITR_MASK) > IECM_ITR_ADAPTIVE_MAX_USECS) {
			new_itr &= IECM_ITR_ADAPTIVE_LATENCY;
			new_itr += IECM_ITR_ADAPTIVE_MAX_USECS;
		}
		goto clear_counts;
	}

	if (packets <= 256) {
		new_itr = min(q_vector->tx[0]->itr.current_itr,
			      q_vector->rx[0]->itr.current_itr);
		new_itr &= IECM_ITR_MASK;

		/* Between 56 and 112 is our "goldilocks" zone where we are
		 * working out "just right". Just report that our current
		 * ITR is good for us.
		 */
		if (packets <= 112)
			goto clear_counts;

		/* If packet count is 128 or greater we are likely looking
		 * at a slight overrun of the delay we want. Try halving
		 * our delay to see if that will cut the number of packets
		 * in half per interrupt.
		 */
		new_itr /= 2;
		new_itr &= IECM_ITR_MASK;
		if (new_itr < IECM_ITR_ADAPTIVE_MIN_USECS)
			new_itr = IECM_ITR_ADAPTIVE_MIN_USECS;

		goto clear_counts;
	}

	/* The paths below assume we are dealing with a bulk ITR since
	 * number of packets is greater than 256. We are just going to have
	 * to compute a value and try to bring the count under control,
	 * though for smaller packet sizes there isn't much we can do as
	 * NAPI polling will likely be kicking in sooner rather than later.
	 */
	new_itr = IECM_ITR_ADAPTIVE_BULK;

adjust_by_size:
	/* If packet counts are 256 or greater we can assume we have a gross
	 * overestimation of what the rate should be. Instead of trying to fine
	 * tune it just use the formula below to try and dial in an exact value
	 * give the current packet size of the frame.
	 */
	avg_wire_size = bytes / packets;

	/* The following is a crude approximation of:
	 *  wmem_default / (size + overhead) = desired_pkts_per_int
	 *  rate / bits_per_byte / (size + Ethernet overhead) = pkt_rate
	 *  (desired_pkt_rate / pkt_rate) * usecs_per_sec = ITR value
	 *
	 * Assuming wmem_default is 212992 and overhead is 640 bytes per
	 * packet, (256 skb, 64 headroom, 320 shared info), we can reduce the
	 * formula down to
	 *
	 *  (170 * (size + 24)) / (size + 640) = ITR
	 *
	 * We first do some math on the packet size and then finally bit shift
	 * by 8 after rounding up. We also have to account for PCIe link speed
	 * difference as ITR scales based on this.
	 */
	if (avg_wire_size <= 60) {
		/* Start at 250k ints/sec */
		avg_wire_size = 4096;
	} else if (avg_wire_size <= 380) {
		/* 250K ints/sec to 60K ints/sec */
		avg_wire_size *= 40;
		avg_wire_size += 1696;
	} else if (avg_wire_size <= 1084) {
		/* 60K ints/sec to 36K ints/sec */
		avg_wire_size *= 15;
		avg_wire_size += 11452;
	} else if (avg_wire_size <= 1980) {
		/* 36K ints/sec to 30K ints/sec */
		avg_wire_size *= 5;
		avg_wire_size += 22420;
	} else {
		/* plateau at a limit of 30K ints/sec */
		avg_wire_size = 32256;
	}

	/* If we are in low latency mode halve our delay which doubles the
	 * rate to somewhere between 100K to 16K ints/sec
	 */
	if (new_itr & IECM_ITR_ADAPTIVE_LATENCY)
		avg_wire_size /= 2;

	/* Resultant value is 256 times larger than it needs to be. This
	 * gives us room to adjust the value as needed to either increase
	 * or decrease the value based on link speeds of 10G, 2.5G, 1G, etc.
	 *
	 * Use addition as we have already recorded the new latency flag
	 * for the ITR value.
	 */
	new_itr += DIV_ROUND_UP(avg_wire_size, iecm_itr_divisor(q_vector)) *
		   IECM_ITR_ADAPTIVE_MIN_INC;

	if ((new_itr & IECM_ITR_MASK) > IECM_ITR_ADAPTIVE_MAX_USECS) {
		new_itr &= IECM_ITR_ADAPTIVE_LATENCY;
		new_itr += IECM_ITR_ADAPTIVE_MAX_USECS;
	}

clear_counts:
	/* write back value */
	itr->target_itr = new_itr;

	/* next update should occur within next jiffy */
	itr->next_update = next_update + 1;

	if (q_type == VIRTCHNL_QUEUE_TYPE_RX) {
		itr->stats.rx.bytes = 0;
		itr->stats.rx.packets = 0;
	} else if (q_type == VIRTCHNL_QUEUE_TYPE_TX) {
		itr->stats.tx.bytes = 0;
		itr->stats.tx.packets = 0;
	}
}

/**
 * iecm_vport_intr_update_itr_ena_irq - Update ITR and re-enable MSIX interrupt
 * @q_vector: q_vector for which ITR is being updated and interrupt enabled
 */
void iecm_vport_intr_update_itr_ena_irq(struct iecm_q_vector *q_vector)
{
	struct iecm_hw *hw = &q_vector->vport->adapter->hw;
	struct iecm_itr *tx_itr = &q_vector->tx[0]->itr;
	struct iecm_itr *rx_itr = &q_vector->rx[0]->itr;
	u32 intval;

	/* These will do nothing if dynamic updates are not enabled */
	iecm_vport_intr_set_new_itr(q_vector, tx_itr, q_vector->tx[0]->q_type);
	iecm_vport_intr_set_new_itr(q_vector, rx_itr, q_vector->rx[0]->q_type);

	/* This block of logic allows us to get away with only updating
	 * one ITR value with each interrupt. The idea is to perform a
	 * pseudo-lazy update with the following criteria.
	 *
	 * 1. Rx is given higher priority than Tx if both are in same state
	 * 2. If we must reduce an ITR that is given highest priority.
	 * 3. We then give priority to increasing ITR based on amount.
	 */
	if (rx_itr->target_itr < rx_itr->current_itr) {
		/* Rx ITR needs to be reduced, this is highest priority */
		intval = iecm_vport_intr_buildreg_itr(q_vector,
						      rx_itr->itr_idx,
						      rx_itr->target_itr);
		rx_itr->current_itr = rx_itr->target_itr;
		q_vector->itr_countdown = ITR_COUNTDOWN_START;
	} else if ((tx_itr->target_itr < tx_itr->current_itr) ||
		   ((rx_itr->target_itr - rx_itr->current_itr) <
		    (tx_itr->target_itr - tx_itr->current_itr))) {
		/* Tx ITR needs to be reduced, this is second priority
		 * Tx ITR needs to be increased more than Rx, fourth priority
		 */
		intval = iecm_vport_intr_buildreg_itr(q_vector,
						      tx_itr->itr_idx,
						      tx_itr->target_itr);
		tx_itr->current_itr = tx_itr->target_itr;
		q_vector->itr_countdown = ITR_COUNTDOWN_START;
	} else if (rx_itr->current_itr != rx_itr->target_itr) {
		/* Rx ITR needs to be increased, third priority */
		intval = iecm_vport_intr_buildreg_itr(q_vector,
						      rx_itr->itr_idx,
						      rx_itr->target_itr);
		rx_itr->current_itr = rx_itr->target_itr;
		q_vector->itr_countdown = ITR_COUNTDOWN_START;
	} else {
		/* No ITR update, lowest priority */
		intval = iecm_vport_intr_buildreg_itr(q_vector,
						      VIRTCHNL_ITR_IDX_NO_ITR,
						      0);
		if (q_vector->itr_countdown)
			q_vector->itr_countdown--;
	}

	wr32(hw, q_vector->intr_reg.dyn_ctl, intval);
}

/**
 * iecm_vport_intr_req_irq - get MSI-X vectors from the OS for the vport
 * @vport: main vport structure
 * @basename: name for the vector
 */
static int
iecm_vport_intr_req_irq(struct iecm_vport *vport, char *basename)
{
	struct iecm_adapter *adapter = vport->adapter;
	int vector, err, irq_num, vidx;

	for (vector = 0; vector < vport->num_q_vectors; vector++) {
		struct iecm_q_vector *q_vector = &vport->q_vectors[vector];

		vidx = vector + vport->q_vector_base;
		irq_num = adapter->msix_entries[vidx].vector;

		snprintf(q_vector->name, sizeof(q_vector->name) - 1,
			 "%s-%s-%d", basename, "TxRx", vidx);

		err = request_irq(irq_num, vport->irq_q_handler, 0,
				  q_vector->name, q_vector);
		if (err) {
			netdev_err(vport->netdev,
				   "Request_irq failed, error: %d\n", err);
			goto free_q_irqs;
		}
		/* assign the mask for this IRQ */
		irq_set_affinity_hint(irq_num, &q_vector->affinity_mask);
	}

	return 0;

free_q_irqs:
	while (vector) {
		vector--;
		vidx = vector + vport->q_vector_base;
		irq_num = adapter->msix_entries[vidx].vector,
		free_irq(irq_num,
			 &vport->q_vectors[vector]);
	}
	return err;
}

/**
 * iecm_vport_intr_ena_irq_all - Enable IRQ for the given vport
 * @vport: main vport structure
 */
void iecm_vport_intr_ena_irq_all(struct iecm_vport *vport)
{
	int q_idx;

	for (q_idx = 0; q_idx < vport->num_q_vectors; q_idx++) {
		struct iecm_q_vector *q_vector = &vport->q_vectors[q_idx];

		if (q_vector->num_txq || q_vector->num_rxq)
			iecm_vport_intr_update_itr_ena_irq(q_vector);
	}
}

/**
 * iecm_vport_intr_deinit - Release all vector associations for the vport
 * @vport: main vport structure
 */
void iecm_vport_intr_deinit(struct iecm_vport *vport)
{
	iecm_vport_intr_napi_dis_all(vport);
	iecm_vport_intr_dis_irq_all(vport);
	iecm_vport_intr_rel_irq(vport);
}

/**
 * iecm_vport_intr_napi_ena_all - Enable NAPI for all q_vectors in the vport
 * @vport: main vport structure
 */
static void
iecm_vport_intr_napi_ena_all(struct iecm_vport *vport)
{
	int q_idx;

	if (!vport->netdev)
		return;

	for (q_idx = 0; q_idx < vport->num_q_vectors; q_idx++) {
		struct iecm_q_vector *q_vector = &vport->q_vectors[q_idx];

		napi_enable(&q_vector->napi);
	}
}

/**
 * iecm_tx_splitq_clean_all- Clean completion queues
 * @q_vec: queue vector
 * @budget: Used to determine if we are in netpoll
 *
 * Returns false if clean is not complete else returns true
 */
static bool
iecm_tx_splitq_clean_all(struct iecm_q_vector *q_vec, int budget)
{
	bool clean_complete = true;
	int i, budget_per_q;

	budget_per_q = max(budget / q_vec->num_txq, 1);
	for (i = 0; i < q_vec->num_txq; i++) {
		if (!iecm_tx_clean_complq(q_vec->tx[i], budget_per_q))
			clean_complete = false;
	}
	return clean_complete;
}

/**
 * iecm_rx_splitq_clean_all- Clean completion queues
 * @q_vec: queue vector
 * @budget: Used to determine if we are in netpoll
 * @cleaned: returns number of packets cleaned
 *
 * Returns false if clean is not complete else returns true
 */
static bool
iecm_rx_splitq_clean_all(struct iecm_q_vector *q_vec, int budget,
			 int *cleaned)
{
	bool clean_complete = true;
	int pkts_cleaned_per_q;
	int i, budget_per_q;

	budget_per_q = max(budget / q_vec->num_rxq, 1);
	for (i = 0; i < q_vec->num_rxq; i++) {
		pkts_cleaned_per_q  = iecm_rx_splitq_clean(q_vec->rx[i],
							   budget_per_q);
		/* if we clean as many as budgeted, we must not
		 * be done
		 */
		if (pkts_cleaned_per_q >= budget_per_q)
			clean_complete = false;
		*cleaned += pkts_cleaned_per_q;
	}
	return clean_complete;
}

/**
 * iecm_vport_splitq_napi_poll - NAPI handler
 * @napi: struct from which you get q_vector
 * @budget: budget provided by stack
 */
static int iecm_vport_splitq_napi_poll(struct napi_struct *napi, int budget)
{
	struct iecm_q_vector *q_vector =
				container_of(napi, struct iecm_q_vector, napi);
	bool clean_complete;
	int work_done = 0;

	clean_complete = iecm_tx_splitq_clean_all(q_vector, budget);

	/* Handle case where we are called by netpoll with a budget of 0 */
	if (budget <= 0)
		return budget;

	/* We attempt to distribute budget to each Rx queue fairly, but don't
	 * allow the budget to go below 1 because that would exit polling early.
	 */
	clean_complete |= iecm_rx_splitq_clean_all(q_vector, budget,
						   &work_done);

	/* If work not completed, return budget and polling will return */
	if (!clean_complete)
		return budget;

	/* Exit the polling mode, but don't re-enable interrupts if stack might
	 * poll us due to busy-polling
	 */
	if (likely(napi_complete_done(napi, work_done)))
		iecm_vport_intr_update_itr_ena_irq(q_vector);

	return min_t(int, work_done, budget - 1);
}

/**
 * iecm_vport_intr_map_vector_to_qs - Map vectors to queues
 * @vport: virtual port
 *
 * Mapping for vectors to queues
 */
static void iecm_vport_intr_map_vector_to_qs(struct iecm_vport *vport)
{
	int i, j, k = 0, num_rxq, num_txq;
	struct iecm_rxq_group *rx_qgrp;
	struct iecm_txq_group *tx_qgrp;
	struct iecm_queue *q;
	int q_index;

	for (i = 0; i < vport->num_rxq_grp; i++) {
		rx_qgrp = &vport->rxq_grps[i];
		if (iecm_is_queue_model_split(vport->rxq_model))
			num_rxq = rx_qgrp->splitq.num_rxq_sets;
		else
			num_rxq = rx_qgrp->singleq.num_rxq;

		for (j = 0; j < num_rxq; j++) {
			if (k >= vport->num_q_vectors)
				k = k % vport->num_q_vectors;

			if (iecm_is_queue_model_split(vport->rxq_model))
				q = &rx_qgrp->splitq.rxq_sets[j].rxq;
			else
				q = &rx_qgrp->singleq.rxqs[j];
			q->q_vector = &vport->q_vectors[k];
			q_index = q->q_vector->num_rxq;
			q->q_vector->rx[q_index] = q;
			q->q_vector->num_rxq++;

			k++;
		}
	}
	k = 0;
	for (i = 0; i < vport->num_txq_grp; i++) {
		tx_qgrp = &vport->txq_grps[i];
		num_txq = tx_qgrp->num_txq;

		if (iecm_is_queue_model_split(vport->txq_model)) {
			if (k >= vport->num_q_vectors)
				k = k % vport->num_q_vectors;

			q = tx_qgrp->complq;
			q->q_vector = &vport->q_vectors[k];
			q_index = q->q_vector->num_txq;
			q->q_vector->tx[q_index] = q;
			q->q_vector->num_txq++;
			k++;
		} else {
			for (j = 0; j < num_txq; j++) {
				if (k >= vport->num_q_vectors)
					k = k % vport->num_q_vectors;

				q = &tx_qgrp->txqs[j];
				q->q_vector = &vport->q_vectors[k];
				q_index = q->q_vector->num_txq;
				q->q_vector->tx[q_index] = q;
				q->q_vector->num_txq++;

				k++;
			}
		}
	}
}

/**
 * iecm_vport_intr_init_vec_idx - Initialize the vector indexes
 * @vport: virtual port
 *
 * Initialize vector indexes with values returned over mailbox
 */
static int iecm_vport_intr_init_vec_idx(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct iecm_q_vector *q_vector;
	int i;

	if (adapter->req_vec_chunks) {
		struct virtchnl_vector_chunks *vchunks;
		struct virtchnl_alloc_vectors *ac;
		/* We may never deal with more that 256 same type of vectors */
#define IECM_MAX_VECIDS	256
		u16 vecids[IECM_MAX_VECIDS];
		int num_ids;

		ac = adapter->req_vec_chunks;
		vchunks = &ac->vchunks;

		num_ids = iecm_vport_get_vec_ids(vecids, IECM_MAX_VECIDS,
						 vchunks);
		if (num_ids != adapter->num_msix_entries)
			return -EFAULT;

		for (i = 0; i < vport->num_q_vectors; i++) {
			q_vector = &vport->q_vectors[i];
			q_vector->v_idx = vecids[i + vport->q_vector_base];
		}
	} else {
		for (i = 0; i < vport->num_q_vectors; i++) {
			q_vector = &vport->q_vectors[i];
			q_vector->v_idx = i + vport->q_vector_base;
		}
	}

	return 0;
}

/**
 * iecm_vport_intr_alloc - Allocate memory for interrupt vectors
 * @vport: virtual port
 *
 * We allocate one q_vector per queue interrupt. If allocation fails we
 * return -ENOMEM.
 */
int iecm_vport_intr_alloc(struct iecm_vport *vport)
{
	int txqs_per_vector, rxqs_per_vector;
	struct iecm_q_vector *q_vector;
	int v_idx, err = 0;

	vport->q_vectors = kcalloc(vport->num_q_vectors,
				   sizeof(struct iecm_q_vector), GFP_KERNEL);

	if (!vport->q_vectors)
		return -ENOMEM;

	txqs_per_vector = DIV_ROUND_UP(vport->num_txq, vport->num_q_vectors);
	rxqs_per_vector = DIV_ROUND_UP(vport->num_rxq, vport->num_q_vectors);

	for (v_idx = 0; v_idx < vport->num_q_vectors; v_idx++) {
		q_vector = &vport->q_vectors[v_idx];
		q_vector->vport = vport;
		q_vector->itr_countdown = ITR_COUNTDOWN_START;

		q_vector->tx = kcalloc(txqs_per_vector,
				       sizeof(struct iecm_queue *),
				       GFP_KERNEL);
		if (!q_vector->tx) {
			err = -ENOMEM;
			goto free_vport_q_vec;
		}

		q_vector->rx = kcalloc(rxqs_per_vector,
				       sizeof(struct iecm_queue *),
				       GFP_KERNEL);
		if (!q_vector->rx) {
			err = -ENOMEM;
			goto free_vport_q_vec_tx;
		}

		/* only set affinity_mask if the CPU is online */
		if (cpu_online(v_idx))
			cpumask_set_cpu(v_idx, &q_vector->affinity_mask);

		/* Register the NAPI handler */
		if (vport->netdev) {
			if (iecm_is_queue_model_split(vport->txq_model))
				netif_napi_add(vport->netdev, &q_vector->napi,
					       iecm_vport_splitq_napi_poll,
					       NAPI_POLL_WEIGHT);
			else
				netif_napi_add(vport->netdev, &q_vector->napi,
					       iecm_vport_singleq_napi_poll,
					       NAPI_POLL_WEIGHT);
		}
	}

	return 0;
free_vport_q_vec_tx:
	kfree(q_vector->tx);
free_vport_q_vec:
	kfree(vport->q_vectors);

	return err;
}

/**
 * iecm_vport_intr_init - Setup all vectors for the given vport
 * @vport: virtual port
 *
 * Returns 0 on success or negative on failure
 */
int iecm_vport_intr_init(struct iecm_vport *vport)
{
	char int_name[IECM_INT_NAME_STR_LEN];
	int err = 0;

	err = iecm_vport_intr_init_vec_idx(vport);
	if (err)
		goto handle_err;

	iecm_vport_intr_map_vector_to_qs(vport);
	iecm_vport_intr_napi_ena_all(vport);

	vport->adapter->dev_ops.reg_ops.intr_reg_init(vport);

	snprintf(int_name, sizeof(int_name) - 1, "%s-%s",
		 dev_driver_string(&vport->adapter->pdev->dev),
		 vport->netdev->name);

	err = iecm_vport_intr_req_irq(vport, int_name);
	if (err)
		goto unroll_vectors_alloc;

	iecm_vport_intr_ena_irq_all(vport);
	goto handle_err;
unroll_vectors_alloc:
	iecm_vport_intr_rel(vport);
handle_err:
	return err;
}
EXPORT_SYMBOL(iecm_vport_calc_num_q_vec);

/**
 * iecm_config_rss - Prepare for RSS
 * @vport: virtual port
 *
 * Return 0 on success, negative on failure
 */
int iecm_config_rss(struct iecm_vport *vport)
{
	int err = iecm_send_get_set_rss_key_msg(vport, false);

	if (!err)
		err = vport->adapter->dev_ops.vc_ops.get_set_rss_lut(vport,
								     false);
	if (!err)
		err = vport->adapter->dev_ops.vc_ops.get_set_rss_hash(vport,
								      false);

	return err;
}

/**
 * iecm_get_rx_qid_list - Create a list of RX QIDs
 * @vport: virtual port
 * @qid_list: list of qids
 *
 * qid_list must be allocated for maximum entries to prevent buffer overflow.
 */
void iecm_get_rx_qid_list(struct iecm_vport *vport, u16 *qid_list)
{
	int i, j, k = 0;

	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];

		if (iecm_is_queue_model_split(vport->rxq_model)) {
			for (j = 0; j < rx_qgrp->splitq.num_rxq_sets; j++)
				qid_list[k++] =
					rx_qgrp->splitq.rxq_sets[j].rxq.q_id;
		} else {
			for (j = 0; j < rx_qgrp->singleq.num_rxq; j++)
				qid_list[k++] = rx_qgrp->singleq.rxqs[j].q_id;
		}
	}
}

/**
 * iecm_fill_dflt_rss_lut - Fill the indirection table with the default values
 * @vport: virtual port structure
 * @qid_list: List of the RX qid's
 *
 * qid_list is created and freed by the caller
 */
void iecm_fill_dflt_rss_lut(struct iecm_vport *vport, u16 *qid_list)
{
	int num_lut_segs, lut_seg, i, k = 0;

	num_lut_segs = vport->adapter->rss_data.rss_lut_size / vport->num_rxq;
	for (lut_seg = 0; lut_seg < num_lut_segs; lut_seg++) {
		for (i = 0; i < vport->num_rxq; i++)
			vport->adapter->rss_data.rss_lut[k++] = qid_list[i];
	}
}

/**
 * iecm_init_rss - Prepare for RSS
 * @vport: virtual port
 *
 * Return 0 on success, negative on failure
 */
int iecm_init_rss(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	u16 *qid_list;

	adapter->rss_data.rss_key = kzalloc(adapter->rss_data.rss_key_size,
					    GFP_KERNEL);
	if (!adapter->rss_data.rss_key)
		return -ENOMEM;
	adapter->rss_data.rss_lut = kzalloc(adapter->rss_data.rss_lut_size,
					    GFP_KERNEL);
	if (!adapter->rss_data.rss_lut) {
		kfree(adapter->rss_data.rss_key);
		adapter->rss_data.rss_key = NULL;
		return -ENOMEM;
	}

	/* Initialize default rss key */
	netdev_rss_key_fill((void *)adapter->rss_data.rss_key,
			    adapter->rss_data.rss_key_size);

	/* Initialize default rss lut */
	if (adapter->rss_data.rss_lut_size % vport->num_rxq) {
		u16 dflt_qid;
		int i;

		/* Set all entries to a default RX queue if the algorithm below
		 * won't fill all entries
		 */
		if (iecm_is_queue_model_split(vport->rxq_model))
			dflt_qid =
				vport->rxq_grps[0].splitq.rxq_sets[0].rxq.q_id;
		else
			dflt_qid =
				vport->rxq_grps[0].singleq.rxqs[0].q_id;

		for (i = 0; i < adapter->rss_data.rss_lut_size; i++)
			adapter->rss_data.rss_lut[i] = dflt_qid;
	}

	qid_list = kcalloc(vport->num_rxq, sizeof(u16), GFP_KERNEL);
	if (!qid_list) {
		kfree(adapter->rss_data.rss_lut);
		adapter->rss_data.rss_lut = NULL;
		kfree(adapter->rss_data.rss_key);
		adapter->rss_data.rss_key = NULL;
		return -ENOMEM;
	}

	iecm_get_rx_qid_list(vport, qid_list);

	/* Fill the default RSS lut values*/
	iecm_fill_dflt_rss_lut(vport, qid_list);

	kfree(qid_list);

	 /* Initialize default rss HASH */
	adapter->rss_data.rss_hash = IECM_DEFAULT_RSS_HASH_EXPANDED;

	return iecm_config_rss(vport);
}

/**
 * iecm_deinit_rss - Prepare for RSS
 * @vport: virtual port
 */
void iecm_deinit_rss(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;

	kfree(adapter->rss_data.rss_key);
	adapter->rss_data.rss_key = NULL;
	kfree(adapter->rss_data.rss_lut);
	adapter->rss_data.rss_lut = NULL;
}
