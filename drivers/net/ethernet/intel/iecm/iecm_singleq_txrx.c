// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Intel Corporation */

#include <linux/prefetch.h>
#include <linux/net/intel/iecm.h>

/**
 * iecm_tx_singleq_build_ctob - populate command tag offset and size
 * @td_cmd: Command to be filled in desc
 * @td_offset: Offset to be filled in desc
 * @size: Size of the buffer
 * @td_tag: VLAN tag to be filled
 *
 * Returns the 64 bit value populated with the input parameters
 */
static __le64
iecm_tx_singleq_build_ctob(u64 td_cmd, u64 td_offset, unsigned int size,
			   u64 td_tag)
{
	return cpu_to_le64(IECM_TX_DESC_DTYPE_DATA |
			   (td_cmd    << IECM_TXD_QW1_CMD_S) |
			   (td_offset << IECM_TXD_QW1_OFFSET_S) |
			   ((u64)size << IECM_TXD_QW1_TX_BUF_SZ_S) |
			   (td_tag    << IECM_TXD_QW1_L2TAG1_S));
}

/**
 * iecm_tx_singleq_csum - Enable Tx checksum offloads
 * @first: pointer to first descriptor
 * @off: pointer to struct that holds offload parameters
 *
 * Returns 0 or error (negative) if checksum offload
 */
static
int iecm_tx_singleq_csum(struct iecm_tx_buf *first,
			 struct iecm_tx_offload_params *off)
{
	u32 l4_len = 0, l3_len = 0, l2_len = 0;
	struct sk_buff *skb = first->skb;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		unsigned char *hdr;
	} l4;
	__be16 frag_off, protocol;
	unsigned char *exthdr;
	u32 offset, cmd = 0;
	u8 l4_proto = 0;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	if (skb->encapsulation)
		return -1;

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);

	/* compute outer L2 header size */
	l2_len = ip.hdr - skb->data;
	offset = (l2_len / 2) << IECM_TX_DESC_LEN_MACLEN_S;

	/* Enable IP checksum offloads */
	protocol = vlan_get_protocol(skb);
	if (protocol == htons(ETH_P_IP)) {
		l4_proto = ip.v4->protocol;
		/* the stack computes the IP header already, the only time we
		 * need the hardware to recompute it is in the case of TSO.
		 */
		if (first->tx_flags & IECM_TX_FLAGS_TSO)
			cmd |= IECM_TX_DESC_CMD_IIPT_IPV4_CSUM;
		else
			cmd |= IECM_TX_DESC_CMD_IIPT_IPV4;

	} else if (protocol == htons(ETH_P_IPV6)) {
		cmd |= IECM_TX_DESC_CMD_IIPT_IPV6;
		exthdr = ip.hdr + sizeof(struct ipv6hdr);
		l4_proto = ip.v6->nexthdr;
		if (l4.hdr != exthdr)
			ipv6_skip_exthdr(skb, exthdr - skb->data, &l4_proto,
					 &frag_off);
	} else {
		return -1;
	}

	/* compute inner L3 header size */
	l3_len = l4.hdr - ip.hdr;
	offset |= (l3_len / 4) << IECM_TX_DESC_LEN_IPLEN_S;

	/* Enable L4 checksum offloads */
	switch (l4_proto) {
	case IPPROTO_TCP:
		/* enable checksum offloads */
		cmd |= IECM_TX_DESC_CMD_L4T_EOFT_TCP;
		l4_len = l4.tcp->doff;
		offset |= l4_len << IECM_TX_DESC_LEN_L4_LEN_S;
		break;
	case IPPROTO_UDP:
		/* enable UDP checksum offload */
		cmd |= IECM_TX_DESC_CMD_L4T_EOFT_UDP;
		l4_len = (sizeof(struct udphdr) >> 2);
		offset |= l4_len << IECM_TX_DESC_LEN_L4_LEN_S;
		break;
	case IPPROTO_SCTP:
		/* enable SCTP checksum offload */
		cmd |= IECM_TX_DESC_CMD_L4T_EOFT_SCTP;
		l4_len = sizeof(struct sctphdr) >> 2;
		offset |= l4_len << IECM_TX_DESC_LEN_L4_LEN_S;
		break;

	default:
		if (first->tx_flags & IECM_TX_FLAGS_TSO)
			return -1;
		skb_checksum_help(skb);
		return 0;
	}

	off->td_cmd |= cmd;
	off->hdr_offsets |= offset;
	return 1;
}

/**
 * iecm_tx_singleq_map - Build the Tx base descriptor
 * @tx_q: queue to send buffer on
 * @first: first buffer info buffer to use
 * @offloads: pointer to struct that holds offload parameters
 *
 * This function loops over the skb data pointed to by *first
 * and gets a physical address for each memory location and programs
 * it and the length into the transmit base mode descriptor.
 */
static void
iecm_tx_singleq_map(struct iecm_queue *tx_q, struct iecm_tx_buf *first,
		    struct iecm_tx_offload_params *offloads)
{
	u32 offsets = offloads->hdr_offsets;
	struct iecm_base_tx_desc *tx_desc;
	u64  td_cmd = offloads->td_cmd;
	unsigned int data_len, size;
	struct iecm_tx_buf *tx_buf;
	u16 i = tx_q->next_to_use;
	struct netdev_queue *nq;
	struct sk_buff *skb;
	skb_frag_t *frag;
	dma_addr_t dma;

	skb = first->skb;

	data_len = skb->data_len;
	size = skb_headlen(skb);

	tx_desc = IECM_BASE_TX_DESC(tx_q, i);

	dma = dma_map_single(tx_q->dev, skb->data, size, DMA_TO_DEVICE);

	tx_buf = first;

	/* write each descriptor with CRC bit */
	if (tx_q->vport->adapter->dev_ops.crc_enable)
		tx_q->vport->adapter->dev_ops.crc_enable(&td_cmd);

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		unsigned int max_data = IECM_TX_MAX_DESC_DATA_ALIGNED;

		if (dma_mapping_error(tx_q->dev, dma))
			goto dma_error;

		/* record length, and DMA address */
		dma_unmap_len_set(tx_buf, len, size);
		dma_unmap_addr_set(tx_buf, dma, dma);

		/* align size to end of page */
		max_data += -dma & (IECM_TX_MAX_READ_REQ_SIZE - 1);
		tx_desc->buf_addr = cpu_to_le64(dma);

		/* account for data chunks larger than the hardware
		 * can handle
		 */
		while (unlikely(size > IECM_TX_MAX_DESC_DATA)) {
			tx_desc->qw1 = iecm_tx_singleq_build_ctob(td_cmd,
								  offsets,
								  size, 0);
			tx_desc++;
			i++;

			if (i == tx_q->desc_count) {
				tx_desc = IECM_BASE_TX_DESC(tx_q, 0);
				i = 0;
			}

			dma += max_data;
			size -= max_data;

			max_data = IECM_TX_MAX_DESC_DATA_ALIGNED;
			tx_desc->buf_addr = cpu_to_le64(dma);
		}

		if (likely(!data_len))
			break;
		tx_desc->qw1 = iecm_tx_singleq_build_ctob(td_cmd, offsets,
							  size, 0);
		tx_desc++;
		i++;

		if (i == tx_q->desc_count) {
			tx_desc = IECM_BASE_TX_DESC(tx_q, 0);
			i = 0;
		}

		size = skb_frag_size(frag);
		data_len -= size;

		dma = skb_frag_dma_map(tx_q->dev, frag, 0, size,
				       DMA_TO_DEVICE);

		tx_buf = &tx_q->tx_buf[i];
	}

	/* record bytecount for BQL */
	nq = netdev_get_tx_queue(tx_q->vport->netdev, tx_q->idx);
	netdev_tx_sent_queue(nq, first->bytecount);

	/* record SW timestamp if HW timestamp is not available */
	skb_tx_timestamp(first->skb);

	/* write last descriptor with RS and EOP bits */
	td_cmd |= (u64)(IECM_TX_DESC_CMD_EOP | IECM_TX_DESC_CMD_RS);

	tx_desc->qw1 = iecm_tx_singleq_build_ctob(td_cmd, offsets, size, 0);

	i++;
	if (i == tx_q->desc_count)
		i = 0;

	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;

	iecm_tx_buf_hw_update(tx_q, i);

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
 * iecm_tx_singleq_frame - Sends buffer on Tx ring using base descriptors
 * @skb: send buffer
 * @tx_q: queue to send buffer on
 *
 * Returns NETDEV_TX_OK if sent, else an error code
 */
static netdev_tx_t
iecm_tx_singleq_frame(struct sk_buff *skb, struct iecm_queue *tx_q)
{
	struct iecm_tx_offload_params offload = {0};
	struct iecm_tx_buf *first;
	unsigned int count;
	int csum;

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

	csum = iecm_tx_singleq_csum(first, &offload);
	if (csum < 0)
		goto out_drop;

	iecm_tx_singleq_map(tx_q, first, &offload);

	return NETDEV_TX_OK;

out_drop:
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

/**
 * iecm_tx_singleq_start - Selects the right Tx queue to send buffer
 * @skb: send buffer
 * @netdev: network interface device structure
 *
 * Returns NETDEV_TX_OK if sent, else an error code
 */
netdev_tx_t iecm_tx_singleq_start(struct sk_buff *skb,
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

	return iecm_tx_singleq_frame(skb, tx_q);
}

/**
 * iecm_tx_singleq_clean - Reclaim resources from queue
 * @tx_q: Tx queue to clean
 * @napi_budget: Used to determine if we are in netpoll
 *
 */
static bool iecm_tx_singleq_clean(struct iecm_queue *tx_q, int napi_budget)
{
	unsigned int budget = tx_q->vport->compln_clean_budget;
	unsigned int total_bytes = 0, total_pkts = 0;
	struct iecm_base_tx_desc *tx_desc;
	s16 ntc = tx_q->next_to_clean;
	struct iecm_tx_buf *tx_buf;
	struct netdev_queue *nq;

	tx_desc = IECM_BASE_TX_DESC(tx_q, ntc);
	tx_buf = &tx_q->tx_buf[ntc];
	ntc -= tx_q->desc_count;

	do {
		struct iecm_base_tx_desc *eop_desc = tx_buf->next_to_watch;

		/* if next_to_watch is not set then no work pending */
		if (!eop_desc)
			break;

		/* prevent any other reads prior to eop_desc */
		smp_rmb();

		/* if the descriptor isn't done, no work yet to do */
		if (!(eop_desc->qw1 &
		      cpu_to_le64(IECM_TX_DESC_DTYPE_DESC_DONE)))
			break;

		/* clear next_to_watch to prevent false hangs */
		tx_buf->next_to_watch = NULL;

		/* update the statistics for this packet */
		total_bytes += tx_buf->bytecount;
		total_pkts += tx_buf->gso_segs;

		/* free the skb */
		napi_consume_skb(tx_buf->skb, napi_budget);

		/* unmap skb header data */
		dma_unmap_single(tx_q->dev,
				 dma_unmap_addr(tx_buf, dma),
				 dma_unmap_len(tx_buf, len),
				 DMA_TO_DEVICE);

		/* clear tx_buf data */
		tx_buf->skb = NULL;
		dma_unmap_len_set(tx_buf, len, 0);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buf++;
			tx_desc++;
			ntc++;
			if (unlikely(!ntc)) {
				ntc -= tx_q->desc_count;
				tx_buf = tx_q->tx_buf;
				tx_desc = IECM_BASE_TX_DESC(tx_q, 0);
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

		tx_buf++;
		tx_desc++;
		ntc++;
		if (unlikely(!ntc)) {
			ntc -= tx_q->desc_count;
			tx_buf = tx_q->tx_buf;
			tx_desc = IECM_BASE_TX_DESC(tx_q, 0);
		}
		/* update budget */
		budget--;
	} while (likely(budget));

	ntc += tx_q->desc_count;
	tx_q->next_to_clean = ntc;
	nq = netdev_get_tx_queue(tx_q->vport->netdev, tx_q->idx);
	netdev_tx_completed_queue(nq, total_pkts, total_bytes);
	tx_q->itr.stats.tx.packets += total_pkts;
	tx_q->itr.stats.tx.bytes += total_bytes;

	u64_stats_update_begin(&tx_q->stats_sync);
	tx_q->q_stats.tx.packets += total_pkts;
	tx_q->q_stats.tx.bytes += total_bytes;
	u64_stats_update_end(&tx_q->stats_sync);

	return !!budget;
}

/**
 * iecm_tx_singleq_clean_all - Clean all Tx queues
 * @q_vec: queue vector
 * @budget: Used to determine if we are in netpoll
 *
 * Returns false if clean is not complete else returns true
 */
static bool
iecm_tx_singleq_clean_all(struct iecm_q_vector *q_vec, int budget)
{
	bool clean_complete = true;
	int i, budget_per_q;

	budget_per_q = max(budget / q_vec->num_txq, 1);
	for (i = 0; i < q_vec->num_txq; i++) {
		if (!iecm_tx_singleq_clean(q_vec->tx[i], budget_per_q))
			clean_complete = false;
	}

	return clean_complete;
}

/**
 * iecm_rx_singleq_test_staterr - tests bits in Rx descriptor
 * status and error fields
 * @rx_desc: pointer to receive descriptor (in le64 format)
 * @stat_err_bits: value to mask
 *
 * This function does some fast chicanery in order to return the
 * value of the mask which is really only used for boolean tests.
 * The status_error_ptype_len doesn't need to be shifted because it begins
 * at offset zero.
 */
static bool
iecm_rx_singleq_test_staterr(struct iecm_singleq_base_rx_desc *rx_desc,
			     const u64 stat_err_bits)
{
	return !!(rx_desc->qword1.status_error_ptype_len &
		  cpu_to_le64(stat_err_bits));
}

/**
 * iecm_rx_singleq_is_non_eop - process handling of non-EOP buffers
 * @rxq: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 */
static bool iecm_rx_singleq_is_non_eop(struct iecm_queue *rxq,
				       struct iecm_singleq_base_rx_desc
				       *rx_desc, struct sk_buff *skb)
{
	/* if we are the last buffer then there is nothing else to do */
 #define IECM_RXD_EOF BIT(IECM_RX_BASE_DESC_STATUS_EOF_S)
	if (likely(iecm_rx_singleq_test_staterr(rx_desc, IECM_RXD_EOF)))
		return false;

	/* place skb in next buffer to be received */
	rxq->rx_buf.buf[rxq->next_to_clean].skb = skb;

	return true;
}

/**
 * iecm_rx_singleq_csum - Indicate in skb if checksum is good
 * @rxq: Rx descriptor ring packet is being transacted on
 * @skb: skb currently being received and modified
 * @rx_desc: the receive descriptor
 * @ptype: the packet type decoded by hardware
 *
 * skb->protocol must be set before this function is called
 */
static void iecm_rx_singleq_csum(struct iecm_queue *rxq, struct sk_buff *skb,
				 struct iecm_singleq_base_rx_desc *rx_desc,
				 u8 ptype)
{
	u64 qw1 = le64_to_cpu(rx_desc->qword1.status_error_ptype_len);
	struct iecm_rx_ptype_decoded decoded;
	bool ipv4, ipv6;
	u32 rx_status;
	u8 rx_error;

	/* Start with CHECKSUM_NONE and by default csum_level = 0 */
	skb->ip_summed = CHECKSUM_NONE;
	skb_checksum_none_assert(skb);

	/* check if Rx checksum is enabled */
	if (!(rxq->vport->netdev->features & NETIF_F_RXCSUM))
		return;

	rx_status = ((qw1 & IECM_RXD_QW1_STATUS_M) >> IECM_RXD_QW1_STATUS_S);
	rx_error = ((qw1 & IECM_RXD_QW1_ERROR_M) >> IECM_RXD_QW1_ERROR_S);

	/* check if HW has decoded the packet and checksum */
	if (!(rx_status & BIT(IECM_RX_BASE_DESC_STATUS_L3L4P_S)))
		return;

	decoded = rxq->vport->rx_ptype_lkup[ptype];
	if (!(decoded.known && decoded.outer_ip))
		return;

	ipv4 = (decoded.outer_ip == IECM_RX_PTYPE_OUTER_IP) &&
	       (decoded.outer_ip_ver == IECM_RX_PTYPE_OUTER_IPV4);
	ipv6 = (decoded.outer_ip == IECM_RX_PTYPE_OUTER_IP) &&
	       (decoded.outer_ip_ver == IECM_RX_PTYPE_OUTER_IPV6);

	if (ipv4 && (rx_error & (BIT(IECM_RX_BASE_DESC_ERROR_IPE_S) |
				 BIT(IECM_RX_BASE_DESC_ERROR_EIPE_S))))
		goto checksum_fail;
	else if (ipv6 && (rx_status &
		 (BIT(IECM_RX_BASE_DESC_STATUS_IPV6EXADD_S))))
		goto checksum_fail;

	/* check for L4 errors and handle packets that were not able to be
	 * checksummed due to arrival speed
	 */
	if (rx_error & BIT(IECM_RX_BASE_DESC_ERROR_L3L4E_S))
		goto checksum_fail;

	/* Only report checksum unnecessary for ICMP, TCP, UDP, or SCTP */
	switch (decoded.inner_prot) {
	case IECM_RX_PTYPE_INNER_PROT_ICMP:
	case IECM_RX_PTYPE_INNER_PROT_TCP:
	case IECM_RX_PTYPE_INNER_PROT_UDP:
	case IECM_RX_PTYPE_INNER_PROT_SCTP:
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	default:
		break;
	}
	return;

checksum_fail:
	rxq->q_stats.rx.csum_err++;
}

/**
 * iecm_rx_singleq_process_skb_fields - Populate skb header fields from Rx
 * descriptor
 * @rxq: Rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being populated
 * @rx_desc: descriptor for skb
 * @ptype: packet type
 *
 * This function checks the ring, descriptor, and packet information in
 * order to populate the hash, checksum, VLAN, protocol, and
 * other fields within the skb.
 */
static void
iecm_rx_singleq_process_skb_fields(struct iecm_queue *rxq, struct sk_buff *skb,
				   struct iecm_singleq_base_rx_desc *rx_desc,
				   u8 ptype)
{
	/* modifies the skb - consumes the enet header */
	skb->protocol = eth_type_trans(skb, rxq->vport->netdev);

	iecm_rx_singleq_csum(rxq, skb, rx_desc, ptype);
}

/**
 * iecm_rx_singleq_buf_hw_alloc_all - Replace used receive buffers
 * @rx_q: queue for which the hw buffers are allocated
 * @cleaned_count: number of buffers to replace
 *
 * Returns false if all allocations were successful, true if any fail
 */
bool iecm_rx_singleq_buf_hw_alloc_all(struct iecm_queue *rx_q,
				      u16 cleaned_count)
{
	struct iecm_singleq_rx_buf_desc *singleq_rx_desc = NULL;
	u16 nta = rx_q->next_to_alloc;
	struct iecm_rx_buf *buf;

	/* do nothing if no valid netdev defined */
	if (!rx_q->vport->netdev || !cleaned_count)
		return false;

	singleq_rx_desc = IECM_SINGLEQ_RX_BUF_DESC(rx_q, nta);
	buf = &rx_q->rx_buf.buf[nta];

	do {
		if (!iecm_rx_buf_hw_alloc(rx_q, buf))
			break;

		/* Refresh the desc even if buffer_addrs didn't change
		 * because each write-back erases this info.
		 */
		singleq_rx_desc->pkt_addr =
			cpu_to_le64(buf->dma + buf->page_offset);
		singleq_rx_desc->hdr_addr = 0;
		singleq_rx_desc++;

		buf++;
		nta++;
		if (unlikely(nta == rx_q->desc_count)) {
			singleq_rx_desc = IECM_SINGLEQ_RX_BUF_DESC(rx_q, 0);
			buf = rx_q->rx_buf.buf;
			nta = 0;
		}

		cleaned_count--;
	} while (cleaned_count);

	if (rx_q->next_to_alloc != nta)
		iecm_rx_buf_hw_update(rx_q, nta);

	return !!cleaned_count;
}

/**
 * iecm_singleq_rx_put_buf - wrapper function to clean and recycle buffers
 * @rx_bufq: Rx descriptor queue to transact packets on
 * @rx_buf: Rx buffer to pull data from
 *
 * This function will update the next_to_use/next_to_alloc if the current
 * buffer is recycled.
 */
static void iecm_singleq_rx_put_buf(struct iecm_queue *rx_bufq,
				    struct iecm_rx_buf *rx_buf)
{
	u16 ntu = rx_bufq->next_to_use;
	bool recycled = false;

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
static void iecm_singleq_rx_bump_ntc(struct iecm_queue *q)
{
	u16 ntc = q->next_to_clean + 1;
	/* fetch, update, and store next to clean */
	if (ntc < q->desc_count)
		q->next_to_clean = ntc;
	else
		q->next_to_clean = 0;
}

/**
 * iecm_singleq_rx_get_buf_page - Fetch Rx buffer page and synchronize data
 * @dev: device struct
 * @rx_buf: Rx buf to fetch page for
 * @size: size of buffer to add to skb
 *
 * This function will pull an Rx buffer page from the ring and synchronize it
 * for use by the CPU.
 */
static struct sk_buff *
iecm_singleq_rx_get_buf_page(struct device *dev, struct iecm_rx_buf *rx_buf,
			     const unsigned int size)
{
	prefetch(rx_buf->page);

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(dev, rx_buf->dma,
				      rx_buf->page_offset, size,
				      DMA_FROM_DEVICE);

	/* We have pulled a buffer for use, so decrement pagecnt_bias */
	rx_buf->pagecnt_bias--;

	return rx_buf->skb;
}

/**
 * iecm_rx_singleq_clean - Reclaim resources after receive completes
 * @rx_q: Rx queue to clean
 * @budget: Total limit on number of packets to process
 *
 * Returns true if there's any budget left (e.g. the clean is finished)
 */
static int iecm_rx_singleq_clean(struct iecm_queue *rx_q, int budget)
{
	struct iecm_singleq_base_rx_desc *singleq_base_rx_desc;
	unsigned int total_rx_bytes = 0, total_rx_pkts = 0;
	u16 cleaned_count = 0;
	bool failure = false;

	/* Process Rx packets bounded by budget */
	while (likely(total_rx_pkts < (unsigned int)budget)) {
		union iecm_rx_desc *rx_desc;
		struct sk_buff *skb = NULL;
		struct iecm_rx_buf *rx_buf;
		unsigned int size;
		u8 rx_ptype;
		u64 qword;

		/* get the Rx desc from Rx queue based on 'next_to_clean' */
		rx_desc = IECM_RX_DESC(rx_q, rx_q->next_to_clean);
		singleq_base_rx_desc = (struct iecm_singleq_base_rx_desc *)
					rx_desc;
		/* status_error_ptype_len will always be zero for unused
		 * descriptors because it's cleared in cleanup, and overlaps
		 * with hdr_addr which is always zero because packet split
		 * isn't used, if the hardware wrote DD then the length will be
		 * non-zero
		 */
		qword =
		le64_to_cpu(rx_desc->base_wb.qword1.status_error_ptype_len);

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc
		 */
		dma_rmb();
#define IECM_RXD_DD BIT(IECM_RX_BASE_DESC_STATUS_DD_S)
		if (!iecm_rx_singleq_test_staterr(singleq_base_rx_desc,
						  IECM_RXD_DD))
			break;

		size = (qword & IECM_RXD_QW1_LEN_PBUF_M) >>
		       IECM_RXD_QW1_LEN_PBUF_S;
		if (!size)
			break;

		rx_buf = &rx_q->rx_buf.buf[rx_q->next_to_clean];
		skb = iecm_singleq_rx_get_buf_page(rx_q->dev, rx_buf, size);

		if (skb)
			iecm_rx_add_frag(rx_buf, skb, size);
		else
			skb = iecm_rx_construct_skb(rx_q, rx_buf, size);

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_buf->pagecnt_bias++;
			break;
		}

		iecm_singleq_rx_put_buf(rx_q, rx_buf);
		iecm_singleq_rx_bump_ntc(rx_q);

		cleaned_count++;

		/* skip if it is non EOP desc */
		if (iecm_rx_singleq_is_non_eop(rx_q, singleq_base_rx_desc,
					       skb))
			continue;

#define IECM_RXD_ERR_S BIT(IECM_RXD_QW1_ERROR_S)
		if (unlikely(iecm_rx_singleq_test_staterr(singleq_base_rx_desc,
							  IECM_RXD_ERR_S))) {
			dev_kfree_skb_any(skb);
			skb = NULL;
			continue;
		}

		/* correct empty headers and pad skb if needed (to make valid
		 * ethernet frame
		 */
		if (iecm_rx_cleanup_headers(skb)) {
			skb = NULL;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		rx_ptype = (qword & IECM_RXD_QW1_PTYPE_M) >>
				IECM_RXD_QW1_PTYPE_S;

		/* protocol */
		iecm_rx_singleq_process_skb_fields(rx_q, skb,
						   singleq_base_rx_desc,
						   rx_ptype);

		/* send completed skb up the stack */
		iecm_rx_skb(rx_q, skb);

		/* update budget accounting */
		total_rx_pkts++;
	}
	if (cleaned_count)
		failure = iecm_rx_singleq_buf_hw_alloc_all(rx_q, cleaned_count);

	rx_q->itr.stats.rx.packets += total_rx_pkts;
	rx_q->itr.stats.rx.bytes += total_rx_bytes;
	u64_stats_update_begin(&rx_q->stats_sync);
	rx_q->q_stats.rx.packets += total_rx_pkts;
	rx_q->q_stats.rx.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_q->stats_sync);

	/* guarantee a trip back through this routine if there was a failure */
	return failure ? budget : (int)total_rx_pkts;
}

/**
 * iecm_rx_singleq_clean_all - Clean all Rx queues
 * @q_vec: queue vector
 * @budget: Used to determine if we are in netpoll
 * @cleaned: returns number of packets cleaned
 *
 * Returns false if clean is not complete else returns true
 */
static bool
iecm_rx_singleq_clean_all(struct iecm_q_vector *q_vec, int budget,
			  int *cleaned)
{
	bool clean_complete = true;
	int pkts_cleaned_per_q;
	int budget_per_q, i;

	budget_per_q = max(budget / q_vec->num_rxq, 1);
	for (i = 0; i < q_vec->num_rxq; i++) {
		pkts_cleaned_per_q = iecm_rx_singleq_clean(q_vec->rx[0],
							   budget_per_q);

		/* if we clean as many as budgeted, we must not be done */
		if (pkts_cleaned_per_q >= budget_per_q)
			clean_complete = false;
		*cleaned += pkts_cleaned_per_q;
	}

	return clean_complete;
}

/**
 * iecm_vport_singleq_napi_poll - NAPI handler
 * @napi: struct from which you get q_vector
 * @budget: budget provided by stack
 */
int iecm_vport_singleq_napi_poll(struct napi_struct *napi, int budget)
{
	struct iecm_q_vector *q_vector =
				container_of(napi, struct iecm_q_vector, napi);
	bool clean_complete;
	int work_done = 0;

	clean_complete = iecm_tx_singleq_clean_all(q_vector, budget);

	/* Handle case where we are called by netpoll with a budget of 0 */
	if (budget <= 0)
		return budget;

	/* We attempt to distribute budget to each Rx queue fairly, but don't
	 * allow the budget to go below 1 because that would exit polling early.
	 */
	clean_complete |= iecm_rx_singleq_clean_all(q_vector, budget,
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
