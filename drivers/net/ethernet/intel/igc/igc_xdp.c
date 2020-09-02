// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020, Intel Corporation. */

#include <linux/bpf_trace.h>

#include "igc.h"
#include "igc_xdp.h"

int igc_xdp_set_prog(struct igc_adapter *adapter, struct bpf_prog *prog,
		     struct netlink_ext_ack *extack)
{
	struct net_device *dev = adapter->netdev;
	bool if_running = netif_running(dev);
	struct bpf_prog *old_prog;

	if (dev->mtu > ETH_DATA_LEN) {
		/* For now, the driver doesn't support XDP functionality with
		 * jumbo frames so we return error.
		 */
		NL_SET_ERR_MSG_MOD(extack, "Jumbo frames not supported");
		return -EOPNOTSUPP;
	}

	if (if_running)
		igc_close(dev);

	old_prog = xchg(&adapter->xdp_prog, prog);
	if (old_prog)
		bpf_prog_put(old_prog);

	if (if_running)
		igc_open(dev);

	netdev_dbg(dev, "XDP program set successfully");
	return 0;
}

static int igc_xdp_init_tx_buffer(struct igc_tx_buffer *buffer,
				  struct xdp_frame *xdpf,
				  struct igc_ring *ring)
{
	dma_addr_t dma;

	dma = dma_map_single(ring->dev, xdpf->data, xdpf->len, DMA_TO_DEVICE);
	if (dma_mapping_error(ring->dev, dma)) {
		netdev_err_once(ring->netdev, "Failed to map DMA for TX\n");
		return -ENOMEM;
	}

	buffer->xdpf = xdpf;
	buffer->tx_flags = IGC_TX_FLAGS_XDP;
	buffer->protocol = 0;
	buffer->bytecount = xdpf->len;
	buffer->gso_segs = 1;
	buffer->time_stamp = jiffies;
	dma_unmap_len_set(buffer, len, xdpf->len);
	dma_unmap_addr_set(buffer, dma, dma);
	return 0;
}

/* This function requires __netif_tx_lock is held by the caller. */
static int igc_xdp_init_tx_descriptor(struct igc_ring *ring,
				      struct xdp_frame *xdpf)
{
	struct igc_tx_buffer *buffer;
	union igc_adv_tx_desc *desc;
	u32 cmd_type, olinfo_status;
	int err;

	if (!igc_desc_unused(ring))
		return -EBUSY;

	buffer = &ring->tx_buffer_info[ring->next_to_use];
	err = igc_xdp_init_tx_buffer(buffer, xdpf, ring);
	if (err)
		return err;

	cmd_type = IGC_ADVTXD_DTYP_DATA | IGC_ADVTXD_DCMD_DEXT |
		   IGC_ADVTXD_DCMD_IFCS | IGC_TXD_DCMD |
		   buffer->bytecount;
	olinfo_status = buffer->bytecount << IGC_ADVTXD_PAYLEN_SHIFT;

	desc = IGC_TX_DESC(ring, ring->next_to_use);
	desc->read.cmd_type_len = cpu_to_le32(cmd_type);
	desc->read.olinfo_status = cpu_to_le32(olinfo_status);
	desc->read.buffer_addr = cpu_to_le64(dma_unmap_addr(buffer, dma));

	netdev_tx_sent_queue(txring_txq(ring), buffer->bytecount);

	buffer->next_to_watch = desc;

	ring->next_to_use = (ring->next_to_use + 1) % ring->count;
	return 0;
}

struct igc_ring *igc_xdp_get_tx_ring(struct igc_adapter *adapter, int cpu)
{
	struct igc_ring *ring;
	int index;

	index = cpu % adapter->num_tx_queues;
	ring = adapter->tx_ring[index];
	return ring;
}

static int igc_xdp_xmit_back(struct igc_adapter *adapter, struct xdp_buff *xdp)
{
	struct xdp_frame *xdpf = xdp_convert_buff_to_frame(xdp);
	int cpu = smp_processor_id();
	struct netdev_queue *nq;
	struct igc_ring *ring;
	int res;

	if (unlikely(!xdpf))
		return -EFAULT;

	ring = igc_xdp_get_tx_ring(adapter, cpu);
	nq = txring_txq(ring);

	__netif_tx_lock(nq, cpu);
	res = igc_xdp_init_tx_descriptor(ring, xdpf);
	__netif_tx_unlock(nq);
	return res;
}

struct sk_buff *igc_xdp_run_prog(struct igc_adapter *adapter,
				 struct xdp_buff *xdp)
{
	struct bpf_prog *prog;
	int res;
	u32 act;

	rcu_read_lock();

	prog = READ_ONCE(adapter->xdp_prog);
	if (!prog)
		goto unlock;

	act = bpf_prog_run_xdp(prog, xdp);
	switch (act) {
	case XDP_PASS:
		res = IGC_XDP_PASS;
		break;
	case XDP_TX:
		if (igc_xdp_xmit_back(adapter, xdp) < 0)
			res = IGC_XDP_CONSUMED;
		else
			res = IGC_XDP_TX;
		break;
	case XDP_REDIRECT:
		if (xdp_do_redirect(adapter->netdev, xdp, prog) < 0)
			res = IGC_XDP_CONSUMED;
		else
			res = IGC_XDP_REDIRECT;
		break;
	default:
		bpf_warn_invalid_xdp_action(act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(adapter->netdev, prog, act);
		fallthrough;
	case XDP_DROP:
		res = IGC_XDP_CONSUMED;
		break;
	}

unlock:
	rcu_read_unlock();
	return ERR_PTR(-res);
}

bool igc_xdp_is_enabled(struct igc_adapter *adapter)
{
	return !!adapter->xdp_prog;
}

int igc_xdp_register_rxq_info(struct igc_ring *ring)
{
	struct net_device *dev = ring->netdev;
	int err;

	err = xdp_rxq_info_reg(&ring->xdp_rxq, dev, ring->queue_index);
	if (err) {
		netdev_err(dev, "Failed to register xdp rxq info\n");
		return err;
	}

	err = xdp_rxq_info_reg_mem_model(&ring->xdp_rxq, MEM_TYPE_PAGE_SHARED,
					 NULL);
	if (err) {
		netdev_err(dev, "Failed to register xdp rxq mem model\n");
		xdp_rxq_info_unreg(&ring->xdp_rxq);
		return err;
	}

	return 0;
}

void igc_xdp_unregister_rxq_info(struct igc_ring *ring)
{
	xdp_rxq_info_unreg_mem_model(&ring->xdp_rxq);
	xdp_rxq_info_unreg(&ring->xdp_rxq);
}
