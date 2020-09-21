// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Intel Corporation */

#include <linux/net/intel/iecm.h>

/* Lookup table mapping the HW PTYPE to the bit field for decoding */
static const
struct iecm_rx_ptype_decoded iecm_rx_ptype_lkup[IECM_RX_SUPP_PTYPE] = {
	/* L2 Packet types */
	IECM_PTT_UNUSED_ENTRY(0),
	IECM_PTT(1, L2, NONE, NOF, NONE, NONE, NOF, NONE, PAY2),
	IECM_PTT(11, L2, NONE, NOF, NONE, NONE, NOF, NONE, NONE),
	IECM_PTT_UNUSED_ENTRY(12),

	/* Non Tunneled IPv4 */
	IECM_PTT(22, IP, IPV4, FRG, NONE, NONE, NOF, NONE, PAY3),
	IECM_PTT(23, IP, IPV4, NOF, NONE, NONE, NOF, NONE, PAY3),
	IECM_PTT(24, IP, IPV4, NOF, NONE, NONE, NOF, UDP,  PAY4),
	IECM_PTT_UNUSED_ENTRY(25),
	IECM_PTT(26, IP, IPV4, NOF, NONE, NONE, NOF, TCP,  PAY4),
	IECM_PTT(27, IP, IPV4, NOF, NONE, NONE, NOF, SCTP, PAY4),
	IECM_PTT(28, IP, IPV4, NOF, NONE, NONE, NOF, ICMP, PAY4),

	/* Non Tunneled IPv6 */
	IECM_PTT(88, IP, IPV6, FRG, NONE, NONE, NOF, NONE, PAY3),
	IECM_PTT(89, IP, IPV6, NOF, NONE, NONE, NOF, NONE, PAY3),
	IECM_PTT(90, IP, IPV6, NOF, NONE, NONE, NOF, UDP,  PAY3),
	IECM_PTT_UNUSED_ENTRY(91),
	IECM_PTT(92, IP, IPV6, NOF, NONE, NONE, NOF, TCP,  PAY4),
	IECM_PTT(93, IP, IPV6, NOF, NONE, NONE, NOF, SCTP, PAY4),
	IECM_PTT(94, IP, IPV6, NOF, NONE, NONE, NOF, ICMP, PAY4),
};

/**
 * iecm_recv_event_msg - Receive virtchnl event message
 * @vport: virtual port structure
 *
 * Receive virtchnl event message
 */
static void iecm_recv_event_msg(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	enum virtchnl_link_speed link_speed;
	struct virtchnl_pf_event *vpe;

	vpe = (struct virtchnl_pf_event *)vport->adapter->vc_msg;

	switch (vpe->event) {
	case VIRTCHNL_EVENT_LINK_CHANGE:
		link_speed = vpe->event_data.link_event.link_speed;
		adapter->link_speed = link_speed;
		if (adapter->link_up !=
		    vpe->event_data.link_event.link_status) {
			adapter->link_up =
				vpe->event_data.link_event.link_status;
			if (adapter->link_up) {
				netif_tx_start_all_queues(vport->netdev);
				netif_carrier_on(vport->netdev);
			} else {
				netif_tx_stop_all_queues(vport->netdev);
				netif_carrier_off(vport->netdev);
			}
		}
		break;
	case VIRTCHNL_EVENT_RESET_IMPENDING:
			mutex_lock(&adapter->reset_lock);
			set_bit(__IECM_HR_CORE_RESET, adapter->flags);
			queue_delayed_work(adapter->vc_event_wq,
					   &adapter->vc_event_task,
					   msecs_to_jiffies(10));
		break;
	default:
		dev_err(&vport->adapter->pdev->dev,
			"Unknown event %d from PF\n", vpe->event);
		break;
	}
	mutex_unlock(&vport->adapter->vc_msg_lock);
}

/**
 * iecm_mb_clean - Reclaim the send mailbox queue entries
 * @adapter: Driver specific private structure
 *
 * Reclaim the send mailbox queue entries to be used to send further messages
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_mb_clean(struct iecm_adapter *adapter)
{
	u16 i, num_q_msg = IECM_DFLT_MBX_Q_LEN;
	struct iecm_ctlq_msg **q_msg;
	struct iecm_dma_mem *dma_mem;
	int err = 0;

	q_msg = kcalloc(num_q_msg, sizeof(struct iecm_ctlq_msg *), GFP_KERNEL);
	if (!q_msg)
		return -ENOMEM;

	err = iecm_ctlq_clean_sq(adapter->hw.asq, &num_q_msg, q_msg);
	if (err)
		goto error;

	for (i = 0; i < num_q_msg; i++) {
		dma_mem = q_msg[i]->ctx.indirect.payload;
		if (dma_mem)
			dmam_free_coherent(&adapter->pdev->dev, dma_mem->size,
					   dma_mem->va, dma_mem->pa);
		kfree(q_msg[i]);
	}
error:
	kfree(q_msg);
	return err;
}

/**
 * iecm_send_mb_msg - Send message over mailbox
 * @adapter: Driver specific private structure
 * @op: virtchnl opcode
 * @msg_size: size of the payload
 * @msg: pointer to buffer holding the payload
 *
 * Will prepare the control queue message and initiates the send API
 *
 * Returns 0 on success, negative on failure
 */
int iecm_send_mb_msg(struct iecm_adapter *adapter, enum virtchnl_ops op,
		     u16 msg_size, u8 *msg)
{
	struct iecm_ctlq_msg *ctlq_msg;
	struct iecm_dma_mem *dma_mem;
	int err = 0;

	err = iecm_mb_clean(adapter);
	if (err)
		return err;

	ctlq_msg = kzalloc(sizeof(*ctlq_msg), GFP_KERNEL);
	if (!ctlq_msg)
		return -ENOMEM;

	dma_mem = kzalloc(sizeof(*dma_mem), GFP_KERNEL);
	if (!dma_mem) {
		err = -ENOMEM;
		goto dma_mem_error;
	}

	memset(ctlq_msg, 0, sizeof(struct iecm_ctlq_msg));
	ctlq_msg->opcode = iecm_mbq_opc_send_msg_to_cp;
	ctlq_msg->func_id = 0;
	ctlq_msg->data_len = msg_size;
	ctlq_msg->cookie.mbx.chnl_opcode = op;
	ctlq_msg->cookie.mbx.chnl_retval = VIRTCHNL_STATUS_SUCCESS;
	dma_mem->size = IECM_DFLT_MBX_BUF_SIZE;
	dma_mem->va = dmam_alloc_coherent(&adapter->pdev->dev, dma_mem->size,
					  &dma_mem->pa, GFP_KERNEL);
	if (!dma_mem->va) {
		err = -ENOMEM;
		goto dma_alloc_error;
	}
	memcpy(dma_mem->va, msg, msg_size);
	ctlq_msg->ctx.indirect.payload = dma_mem;

	err = iecm_ctlq_send(&adapter->hw, adapter->hw.asq, 1, ctlq_msg);
	if (err)
		goto send_error;

	return 0;
send_error:
	dmam_free_coherent(&adapter->pdev->dev, dma_mem->size, dma_mem->va,
			   dma_mem->pa);
dma_alloc_error:
	kfree(dma_mem);
dma_mem_error:
	kfree(ctlq_msg);
	return err;
}
EXPORT_SYMBOL(iecm_send_mb_msg);

/**
 * iecm_recv_mb_msg - Receive message over mailbox
 * @adapter: Driver specific private structure
 * @op: virtchnl operation code
 * @msg: Received message holding buffer
 * @msg_size: message size
 *
 * Will receive control queue message and posts the receive buffer. Returns 0
 * on success and negative on failure.
 */
int iecm_recv_mb_msg(struct iecm_adapter *adapter, enum virtchnl_ops op,
		     void *msg, int msg_size)
{
	struct iecm_ctlq_msg ctlq_msg;
	struct iecm_dma_mem *dma_mem;
	struct iecm_vport *vport;
	bool work_done = false;
	int payload_size = 0;
	int num_retry = 10;
	u16 num_q_msg;
	int err = 0;

	vport = adapter->vports[0];
	while (1) {
		/* Try to get one message */
		num_q_msg = 1;
		dma_mem = NULL;
		err = iecm_ctlq_recv(adapter->hw.arq, &num_q_msg, &ctlq_msg);
		/* If no message then decide if we have to retry based on
		 * opcode
		 */
		if (err || !num_q_msg) {
			if (op && num_retry--) {
				msleep(20);
				continue;
			} else {
				break;
			}
		}

		/* If we are here a message is received. Check if we are looking
		 * for a specific message based on opcode. If it is different
		 * ignore and post buffers
		 */
		if (op && ctlq_msg.cookie.mbx.chnl_opcode != op)
			goto post_buffs;

		if (ctlq_msg.data_len)
			payload_size = ctlq_msg.ctx.indirect.payload->size;

		/* All conditions are met. Either a message requested is
		 * received or we received a message to be processed
		 */
		switch (ctlq_msg.cookie.mbx.chnl_opcode) {
		case VIRTCHNL_OP_VERSION:
		case VIRTCHNL_OP_GET_CAPS:
		case VIRTCHNL_OP_CREATE_VPORT:
			if (msg)
				memcpy(msg, ctlq_msg.ctx.indirect.payload->va,
				       min_t(int,
					     payload_size, msg_size));
			work_done = true;
			break;
		case VIRTCHNL_OP_ENABLE_VPORT:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_ENA_VPORT_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_ENA_VPORT, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_DISABLE_VPORT:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_DIS_VPORT_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_DIS_VPORT, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_DESTROY_VPORT:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_DESTROY_VPORT_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_DESTROY_VPORT, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_CONFIG_TX_QUEUES:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_CONFIG_TXQ_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_CONFIG_TXQ, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_CONFIG_RX_QUEUES:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_CONFIG_RXQ_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_CONFIG_RXQ, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_ENABLE_QUEUES_V2:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_ENA_QUEUES_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_ENA_QUEUES, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_DISABLE_QUEUES_V2:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_DIS_QUEUES_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_DIS_QUEUES, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_ADD_QUEUES:
			if (ctlq_msg.cookie.mbx.chnl_retval) {
				set_bit(IECM_VC_ADD_QUEUES_ERR,
					adapter->vc_state);
			} else {
				mutex_lock(&adapter->vc_msg_lock);
				memcpy(adapter->vc_msg,
				       ctlq_msg.ctx.indirect.payload->va,
				       min((int)
					   ctlq_msg.ctx.indirect.payload->size,
					   IECM_DFLT_MBX_BUF_SIZE));
			}
			set_bit(IECM_VC_ADD_QUEUES, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_DEL_QUEUES:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_DEL_QUEUES_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_DEL_QUEUES, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_MAP_QUEUE_VECTOR:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_MAP_IRQ_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_MAP_IRQ, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_UNMAP_QUEUE_VECTOR:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_UNMAP_IRQ_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_UNMAP_IRQ, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_GET_STATS:
			if (ctlq_msg.cookie.mbx.chnl_retval) {
				set_bit(IECM_VC_GET_STATS_ERR,
					adapter->vc_state);
			} else {
				mutex_lock(&adapter->vc_msg_lock);
				memcpy(adapter->vc_msg,
				       ctlq_msg.ctx.indirect.payload->va,
				       min_t(int,
					     payload_size,
					     IECM_DFLT_MBX_BUF_SIZE));
			}
			set_bit(IECM_VC_GET_STATS, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_GET_RSS_HASH:
			if (ctlq_msg.cookie.mbx.chnl_retval) {
				set_bit(IECM_VC_GET_RSS_HASH_ERR,
					adapter->vc_state);
			} else {
				mutex_lock(&adapter->vc_msg_lock);
				memcpy(adapter->vc_msg,
				       ctlq_msg.ctx.indirect.payload->va,
				       min_t(int,
					     payload_size,
					     IECM_DFLT_MBX_BUF_SIZE));
			}
			set_bit(IECM_VC_GET_RSS_HASH, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_SET_RSS_HASH:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_SET_RSS_HASH_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_SET_RSS_HASH, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_GET_RSS_LUT:
			if (ctlq_msg.cookie.mbx.chnl_retval) {
				set_bit(IECM_VC_GET_RSS_LUT_ERR,
					adapter->vc_state);
			} else {
				mutex_lock(&adapter->vc_msg_lock);
				memcpy(adapter->vc_msg,
				       ctlq_msg.ctx.indirect.payload->va,
				       min_t(int,
					     payload_size,
					     IECM_DFLT_MBX_BUF_SIZE));
			}
			set_bit(IECM_VC_GET_RSS_LUT, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_SET_RSS_LUT:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_SET_RSS_LUT_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_SET_RSS_LUT, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_GET_RSS_KEY:
			if (ctlq_msg.cookie.mbx.chnl_retval) {
				set_bit(IECM_VC_GET_RSS_KEY_ERR,
					adapter->vc_state);
			} else {
				mutex_lock(&adapter->vc_msg_lock);
				memcpy(adapter->vc_msg,
				       ctlq_msg.ctx.indirect.payload->va,
				       min_t(int,
					     payload_size,
					     IECM_DFLT_MBX_BUF_SIZE));
			}
			set_bit(IECM_VC_GET_RSS_KEY, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_CONFIG_RSS_KEY:
			if (ctlq_msg.cookie.mbx.chnl_retval)
				set_bit(IECM_VC_CONFIG_RSS_KEY_ERR,
					adapter->vc_state);
			set_bit(IECM_VC_CONFIG_RSS_KEY, adapter->vc_state);
			wake_up(&adapter->vchnl_wq);
			break;
		case VIRTCHNL_OP_EVENT:
			mutex_lock(&adapter->vc_msg_lock);
			memcpy(adapter->vc_msg,
			       ctlq_msg.ctx.indirect.payload->va,
			       min_t(int,
				     payload_size,
				     IECM_DFLT_MBX_BUF_SIZE));
			iecm_recv_event_msg(vport);
			break;
		default:
			if (adapter->dev_ops.vc_ops.recv_mbx_msg)
				err =
				adapter->dev_ops.vc_ops.recv_mbx_msg(adapter,
								   msg,
								   msg_size,
								   &ctlq_msg,
								   &work_done);
			else
				dev_warn(&adapter->pdev->dev,
					 "Unhandled virtchnl response %d\n",
					 ctlq_msg.cookie.mbx.chnl_opcode);
			break;
		} /* switch v_opcode */
post_buffs:
		if (ctlq_msg.data_len)
			dma_mem = ctlq_msg.ctx.indirect.payload;
		else
			num_q_msg = 0;

		err = iecm_ctlq_post_rx_buffs(&adapter->hw, adapter->hw.arq,
					      &num_q_msg, &dma_mem);

		/* If post failed clear the only buffer we supplied */
		if (err && dma_mem)
			dmam_free_coherent(&adapter->pdev->dev, dma_mem->size,
					   dma_mem->va, dma_mem->pa);
		/* Applies only if we are looking for a specific opcode */
		if (work_done)
			break;
	}

	return err;
}
EXPORT_SYMBOL(iecm_recv_mb_msg);

/**
 * iecm_send_ver_msg - send virtchnl version message
 * @adapter: Driver specific private structure
 *
 * Send virtchnl version message.  Returns 0 on success, negative on failure.
 */
static int iecm_send_ver_msg(struct iecm_adapter *adapter)
{
	struct virtchnl_version_info vvi;

	vvi.major = VIRTCHNL_VERSION_MAJOR;
	vvi.minor = VIRTCHNL_VERSION_MINOR;

	return iecm_send_mb_msg(adapter, VIRTCHNL_OP_VERSION, sizeof(vvi),
				(u8 *)&vvi);
}

/**
 * iecm_recv_ver_msg - Receive virtchnl version message
 * @adapter: Driver specific private structure
 *
 * Receive virtchnl version message. Returns 0 on success, negative on failure.
 */
static int iecm_recv_ver_msg(struct iecm_adapter *adapter)
{
	struct virtchnl_version_info vvi;
	int err = 0;

	err = iecm_recv_mb_msg(adapter, VIRTCHNL_OP_VERSION, &vvi, sizeof(vvi));
	if (err)
		return err;

	if (vvi.major > VIRTCHNL_VERSION_MAJOR ||
	    (vvi.major == VIRTCHNL_VERSION_MAJOR &&
	     vvi.minor > VIRTCHNL_VERSION_MINOR))
		dev_warn(&adapter->pdev->dev, "Virtchnl version not matched\n");

	return 0;
}

/**
 * iecm_send_get_caps_msg - Send virtchnl get capabilities message
 * @adapter: Driver specific private structure
 *
 * Send virtchl get capabilities message. Returns 0 on success, negative on
 * failure.
 */
int iecm_send_get_caps_msg(struct iecm_adapter *adapter)
{
	struct virtchnl_get_capabilities caps = {0};
	int buf_size;

	buf_size = sizeof(struct virtchnl_get_capabilities);
	adapter->caps = kzalloc(buf_size, GFP_KERNEL);
	if (!adapter->caps)
		return -ENOMEM;

	caps.cap_flags = VIRTCHNL_CAP_STATELESS_OFFLOADS |
	       VIRTCHNL_CAP_UDP_SEG_OFFLOAD |
	       VIRTCHNL_CAP_RSS |
	       VIRTCHNL_CAP_TCP_RSC |
	       VIRTCHNL_CAP_HEADER_SPLIT |
	       VIRTCHNL_CAP_RDMA |
	       VIRTCHNL_CAP_SRIOV |
	       VIRTCHNL_CAP_EDT;

	return iecm_send_mb_msg(adapter, VIRTCHNL_OP_GET_CAPS, sizeof(caps),
				(u8 *)&caps);
}
EXPORT_SYMBOL(iecm_send_get_caps_msg);

/**
 * iecm_recv_get_caps_msg - Receive virtchnl get capabilities message
 * @adapter: Driver specific private structure
 *
 * Receive virtchnl get capabilities message.	Returns 0 on succes, negative on
 * failure.
 */
static int iecm_recv_get_caps_msg(struct iecm_adapter *adapter)
{
	return iecm_recv_mb_msg(adapter, VIRTCHNL_OP_GET_CAPS, adapter->caps,
				sizeof(struct virtchnl_get_capabilities));
}

/**
 * iecm_send_create_vport_msg - Send virtchnl create vport message
 * @adapter: Driver specific private structure
 *
 * send virtchnl create vport message
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_send_create_vport_msg(struct iecm_adapter *adapter)
{
	struct virtchnl_create_vport *vport_msg;
	int buf_size;

	buf_size = sizeof(struct virtchnl_create_vport);
	if (!adapter->vport_params_reqd[0]) {
		adapter->vport_params_reqd[0] = kzalloc(buf_size, GFP_KERNEL);
		if (!adapter->vport_params_reqd[0])
			return -ENOMEM;
	}

	vport_msg = (struct virtchnl_create_vport *)
			adapter->vport_params_reqd[0];
	vport_msg->vport_type = VIRTCHNL_VPORT_TYPE_DEFAULT;
	vport_msg->txq_model = VIRTCHNL_QUEUE_MODEL_SPLIT;
	vport_msg->rxq_model = VIRTCHNL_QUEUE_MODEL_SPLIT;
	iecm_vport_calc_total_qs(adapter, vport_msg);

	return iecm_send_mb_msg(adapter, VIRTCHNL_OP_CREATE_VPORT, buf_size,
				(u8 *)vport_msg);
}

/**
 * iecm_recv_create_vport_msg - Receive virtchnl create vport message
 * @adapter: Driver specific private structure
 * @vport_id: Virtual port identifier
 *
 * Receive virtchnl create vport message.  Returns 0 on success, negative on
 * failure.
 */
static int iecm_recv_create_vport_msg(struct iecm_adapter *adapter,
				      int *vport_id)
{
	struct virtchnl_create_vport *vport_msg;
	int err;

	if (!adapter->vport_params_recvd[0]) {
		adapter->vport_params_recvd[0] = kzalloc(IECM_DFLT_MBX_BUF_SIZE,
							 GFP_KERNEL);
		if (!adapter->vport_params_recvd[0])
			return -ENOMEM;
	}

	vport_msg = (struct virtchnl_create_vport *)
			adapter->vport_params_recvd[0];

	err = iecm_recv_mb_msg(adapter, VIRTCHNL_OP_CREATE_VPORT, vport_msg,
			       IECM_DFLT_MBX_BUF_SIZE);
	if (err)
		return err;
	*vport_id = vport_msg->vport_id;
	return 0;
}

/**
 * iecm_wait_for_event - wait for virtchnl response
 * @adapter: Driver private data structure
 * @state: check on state upon timeout after 500ms
 * @err_check: check if this specific error bit is set
 *
 * Checks if state is set upon expiry of timeout.  Returns 0 on success,
 * negative on failure.
 */
int iecm_wait_for_event(struct iecm_adapter *adapter,
			enum iecm_vport_vc_state state,
			enum iecm_vport_vc_state err_check)
{
	int event;

	event = wait_event_timeout(adapter->vchnl_wq,
				   test_and_clear_bit(state, adapter->vc_state),
				   msecs_to_jiffies(500));
	if (event) {
		if (test_and_clear_bit(err_check, adapter->vc_state)) {
			dev_err(&adapter->pdev->dev,
				"VC response error %d\n", err_check);
			return -EINVAL;
		}
		return 0;
	}

	/* Timeout occurred */
	dev_err(&adapter->pdev->dev, "VC timeout, state = %u\n", state);
	return -ETIMEDOUT;
}
EXPORT_SYMBOL(iecm_wait_for_event);

/**
 * iecm_send_destroy_vport_msg - Send virtchnl destroy vport message
 * @vport: virtual port data structure
 *
 * Send virtchnl destroy vport message.  Returns 0 on success, negative on
 * failure.
 */
int iecm_send_destroy_vport_msg(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_vport v_id;
	int err;

	v_id.vport_id = vport->vport_id;

	err = iecm_send_mb_msg(adapter, VIRTCHNL_OP_DESTROY_VPORT,
			       sizeof(v_id), (u8 *)&v_id);

	if (err)
		return err;
	return iecm_wait_for_event(adapter, IECM_VC_DESTROY_VPORT,
				   IECM_VC_DESTROY_VPORT_ERR);
}

/**
 * iecm_send_enable_vport_msg - Send virtchnl enable vport message
 * @vport: virtual port data structure
 *
 * Send enable vport virtchnl message.	 Returns 0 on success, negative on
 * failure.
 */
int iecm_send_enable_vport_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_disable_vport_msg - Send virtchnl disable vport message
 * @vport: virtual port data structure
 *
 * Send disable vport virtchnl message.  Returns 0 on success, negative on
 * failure.
 */
int iecm_send_disable_vport_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_config_tx_queues_msg - Send virtchnl config Tx queues message
 * @vport: virtual port data structure
 *
 * Send config tx queues virtchnl message. Returns 0 on success, negative on
 * failure.
 */
int iecm_send_config_tx_queues_msg(struct iecm_vport *vport)
{
	struct virtchnl_config_tx_queues *ctq = NULL;
	struct virtchnl_txq_info_v2 *qi;
	int totqs, num_msgs;
	int num_qs, err = 0;
	int i, k = 0;

	totqs = vport->num_txq + vport->num_complq;
	qi = kcalloc(totqs, sizeof(struct virtchnl_txq_info_v2), GFP_KERNEL);
	if (!qi)
		return -ENOMEM;

	/* Populate the queue info buffer with all queue context info */
	for (i = 0; i < vport->num_txq_grp; i++) {
		struct iecm_txq_group *tx_qgrp = &vport->txq_grps[i];
		int j;

		for (j = 0; j < tx_qgrp->num_txq; j++, k++) {
			qi[k].queue_id = tx_qgrp->txqs[j].q_id;
			qi[k].model = vport->txq_model;
			qi[k].type = tx_qgrp->txqs[j].q_type;
			qi[k].ring_len = tx_qgrp->txqs[j].desc_count;
			qi[k].dma_ring_addr = tx_qgrp->txqs[j].dma;
			if (iecm_is_queue_model_split(vport->txq_model)) {
				struct iecm_queue *q = &tx_qgrp->txqs[j];

				qi[k].tx_compl_queue_id = tx_qgrp->complq->q_id;
				qi[k].desc_profile =
					VIRTCHNL_TXQ_DESC_PROFILE_NATIVE;

				if (test_bit(__IECM_Q_FLOW_SCH_EN, q->flags))
					qi[k].sched_mode =
						VIRTCHNL_TXQ_SCHED_MODE_FLOW;
				else
					qi[k].sched_mode =
						VIRTCHNL_TXQ_SCHED_MODE_QUEUE;
			} else {
				qi[k].sched_mode =
					VIRTCHNL_TXQ_SCHED_MODE_QUEUE;
				qi[k].desc_profile =
					VIRTCHNL_TXQ_DESC_PROFILE_BASE;
			}
		}

		if (iecm_is_queue_model_split(vport->txq_model)) {
			qi[k].queue_id = tx_qgrp->complq->q_id;
			qi[k].model = vport->txq_model;
			qi[k].type = tx_qgrp->complq->q_type;
			qi[k].desc_profile = VIRTCHNL_TXQ_DESC_PROFILE_NATIVE;
			qi[k].ring_len = tx_qgrp->complq->desc_count;
			qi[k].dma_ring_addr = tx_qgrp->complq->dma;
			k++;
		}
	}

	/* Make sure accounting agrees */
	if (k != totqs) {
		err = -EINVAL;
		goto error;
	}

	/* Chunk up the queue contexts into multiple messages to avoid
	 * sending a control queue message buffer that is too large
	 */
	if (totqs < IECM_NUM_QCTX_PER_MSG)
		num_qs = totqs;
	else
		num_qs = IECM_NUM_QCTX_PER_MSG;

	num_msgs = totqs / IECM_NUM_QCTX_PER_MSG;
	if (totqs % IECM_NUM_QCTX_PER_MSG)
		num_msgs++;

	for (i = 0, k = 0; i < num_msgs || num_qs; i++) {
		int buf_size = sizeof(struct virtchnl_config_tx_queues) +
			   (sizeof(struct virtchnl_txq_info_v2) * (num_qs - 1));
		if (!ctq || num_qs != IECM_NUM_QCTX_PER_MSG) {
			kfree(ctq);
			ctq = kzalloc(buf_size, GFP_KERNEL);
			if (!ctq) {
				err = -ENOMEM;
				goto error;
			}
		} else {
			memset(ctq, 0, buf_size);
		}

		ctq->vport_id = vport->vport_id;
		ctq->num_qinfo = num_qs;
		memcpy(ctq->qinfo, &qi[k],
		       sizeof(struct virtchnl_txq_info_v2) * num_qs);

		err = iecm_send_mb_msg(vport->adapter,
				       VIRTCHNL_OP_CONFIG_TX_QUEUES,
				       buf_size, (u8 *)ctq);
		if (err)
			goto mbx_error;

		err = iecm_wait_for_event(vport->adapter, IECM_VC_CONFIG_TXQ,
					  IECM_VC_CONFIG_TXQ_ERR);
		if (err)
			goto mbx_error;

		k += num_qs;
		totqs -= num_qs;
		if (totqs < IECM_NUM_QCTX_PER_MSG)
			num_qs = totqs;
	}

mbx_error:
	kfree(ctq);
error:
	kfree(qi);
	return err;
}

/**
 * iecm_send_config_rx_queues_msg - Send virtchnl config Rx queues message
 * @vport: virtual port data structure
 *
 * Send config rx queues virtchnl message.  Returns 0 on success, negative on
 * failure.
 */
int iecm_send_config_rx_queues_msg(struct iecm_vport *vport)
{
	struct virtchnl_config_rx_queues *crq = NULL;
	int totqs, num_msgs, num_qs, err = 0;
	struct virtchnl_rxq_info_v2 *qi;
	int i, k = 0;

	totqs = vport->num_rxq + vport->num_bufq;
	qi = kcalloc(totqs, sizeof(struct virtchnl_rxq_info_v2), GFP_KERNEL);
	if (!qi)
		return -ENOMEM;

	/* Populate the queue info buffer with all queue context info */
	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];
		int num_rxq;
		int j;

		if (iecm_is_queue_model_split(vport->rxq_model))
			num_rxq = rx_qgrp->splitq.num_rxq_sets;
		else
			num_rxq = rx_qgrp->singleq.num_rxq;

		for (j = 0; j < num_rxq; j++, k++) {
			struct iecm_queue *rxq;

			if (iecm_is_queue_model_split(vport->rxq_model)) {
				rxq = &rx_qgrp->splitq.rxq_sets[j].rxq;
				qi[k].rx_bufq1_id =
				  rxq->rxq_grp->splitq.bufq_sets[0].bufq.q_id;
				qi[k].rx_bufq2_id =
				  rxq->rxq_grp->splitq.bufq_sets[1].bufq.q_id;
				qi[k].hdr_buffer_size = rxq->rx_hbuf_size;
				qi[k].rsc_low_watermark =
					rxq->rsc_low_watermark;

				if (rxq->rx_hsplit_en) {
					qi[k].queue_flags =
						VIRTCHNL_RXQ_HDR_SPLIT;
					qi[k].hdr_buffer_size =
						rxq->rx_hbuf_size;
				}
				if (iecm_is_feature_ena(vport, NETIF_F_GRO_HW))
					qi[k].queue_flags |= VIRTCHNL_RXQ_RSC;
			} else {
				rxq = &rx_qgrp->singleq.rxqs[j];
			}

			qi[k].queue_id = rxq->q_id;
			qi[k].model = vport->rxq_model;
			qi[k].type = rxq->q_type;
			qi[k].desc_profile = VIRTCHNL_TXQ_DESC_PROFILE_BASE;
			qi[k].desc_size = VIRTCHNL_RXQ_DESC_SIZE_32BYTE;
			qi[k].ring_len = rxq->desc_count;
			qi[k].dma_ring_addr = rxq->dma;
			qi[k].max_pkt_size = rxq->rx_max_pkt_size;
			qi[k].data_buffer_size = rxq->rx_buf_size;
		}

		if (iecm_is_queue_model_split(vport->rxq_model)) {
			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++, k++) {
				struct iecm_queue *bufq =
					&rx_qgrp->splitq.bufq_sets[j].bufq;

				qi[k].queue_id = bufq->q_id;
				qi[k].model = vport->rxq_model;
				qi[k].type = bufq->q_type;
				qi[k].desc_profile =
					VIRTCHNL_TXQ_DESC_PROFILE_NATIVE;
				qi[k].desc_size =
					VIRTCHNL_RXQ_DESC_SIZE_32BYTE;
				qi[k].ring_len = bufq->desc_count;
				qi[k].dma_ring_addr = bufq->dma;
				qi[k].data_buffer_size = bufq->rx_buf_size;
				qi[k].buffer_notif_stride =
					bufq->rx_buf_stride;
				qi[k].rsc_low_watermark =
					bufq->rsc_low_watermark;
			}
		}
	}

	/* Make sure accounting agrees */
	if (k != totqs) {
		err = -EINVAL;
		goto error;
	}

	/* Chunk up the queue contexts into multiple messages to avoid
	 * sending a control queue message buffer that is too large
	 */
	if (totqs < IECM_NUM_QCTX_PER_MSG)
		num_qs = totqs;
	else
		num_qs = IECM_NUM_QCTX_PER_MSG;

	num_msgs = totqs / IECM_NUM_QCTX_PER_MSG;
	if (totqs % IECM_NUM_QCTX_PER_MSG)
		num_msgs++;

	for (i = 0, k = 0; i < num_msgs || num_qs; i++) {
		int buf_size = sizeof(struct virtchnl_config_rx_queues) +
			   (sizeof(struct virtchnl_rxq_info_v2) * (num_qs - 1));
		if (!crq || num_qs != IECM_NUM_QCTX_PER_MSG) {
			kfree(crq);
			crq = kzalloc(buf_size, GFP_KERNEL);
			if (!crq) {
				err = -ENOMEM;
				goto error;
			}
		} else {
			memset(crq, 0, buf_size);
		}

		crq->vport_id = vport->vport_id;
		crq->num_qinfo = num_qs;
		memcpy(crq->qinfo, &qi[k],
		       sizeof(struct virtchnl_rxq_info_v2) * num_qs);

		err = iecm_send_mb_msg(vport->adapter,
				       VIRTCHNL_OP_CONFIG_RX_QUEUES,
				       buf_size, (u8 *)crq);
		if (err)
			goto mbx_error;

		err = iecm_wait_for_event(vport->adapter, IECM_VC_CONFIG_RXQ,
					  IECM_VC_CONFIG_RXQ_ERR);
		if (err)
			goto mbx_error;

		k += num_qs;
		totqs -= num_qs;
		if (totqs < IECM_NUM_QCTX_PER_MSG)
			num_qs = totqs;
	}

mbx_error:
	kfree(crq);
error:
	kfree(qi);
	return err;
}

/**
 * iecm_send_ena_dis_queues_msg - Send virtchnl enable or disable
 * queues message
 * @vport: virtual port data structure
 * @vc_op: virtchnl op code to send
 *
 * Send enable or disable queues virtchnl message. Returns 0 on success,
 * negative on failure.
 */
static int iecm_send_ena_dis_queues_msg(struct iecm_vport *vport,
					enum virtchnl_ops vc_op)
{
	int num_txq, num_rxq, num_q, buf_size, err = 0;
	struct virtchnl_del_ena_dis_queues *eq;
	struct virtchnl_queue_chunk *qc;
	int i, j, k = 0;

	/* validate virtchnl op */
	switch (vc_op) {
	case VIRTCHNL_OP_ENABLE_QUEUES_V2:
	case VIRTCHNL_OP_DISABLE_QUEUES_V2:
		break;
	default:
		return -EINVAL;
	}

	num_txq = vport->num_txq + vport->num_complq;
	num_rxq = vport->num_rxq + vport->num_bufq;
	num_q = num_txq + num_rxq;
	buf_size = sizeof(struct virtchnl_del_ena_dis_queues) +
		       (sizeof(struct virtchnl_queue_chunk) * (num_q - 1));
	eq = kzalloc(buf_size, GFP_KERNEL);
	if (!eq)
		return -ENOMEM;

	eq->vport_id = vport->vport_id;
	eq->chunks.num_chunks = num_q;
	qc = eq->chunks.chunks;

	for (i = 0; i < vport->num_txq_grp; i++) {
		struct iecm_txq_group *tx_qgrp = &vport->txq_grps[i];

		for (j = 0; j < tx_qgrp->num_txq; j++, k++) {
			qc[k].type = tx_qgrp->txqs[j].q_type;
			qc[k].start_queue_id = tx_qgrp->txqs[j].q_id;
			qc[k].num_queues = 1;
		}
	}
	if (vport->num_txq != k) {
		err = -EINVAL;
		goto error;
	}

	if (iecm_is_queue_model_split(vport->txq_model)) {
		for (i = 0; i < vport->num_txq_grp; i++, k++) {
			struct iecm_txq_group *tx_qgrp = &vport->txq_grps[i];

			qc[k].type = tx_qgrp->complq->q_type;
			qc[k].start_queue_id = tx_qgrp->complq->q_id;
			qc[k].num_queues = 1;
		}
		if (vport->num_complq != (k - vport->num_txq)) {
			err = -EINVAL;
			goto error;
		}
	}

	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];

		if (iecm_is_queue_model_split(vport->rxq_model))
			num_rxq = rx_qgrp->splitq.num_rxq_sets;
		else
			num_rxq = rx_qgrp->singleq.num_rxq;

		for (j = 0; j < num_rxq; j++, k++) {
			if (iecm_is_queue_model_split(vport->rxq_model)) {
				qc[k].start_queue_id =
					rx_qgrp->splitq.rxq_sets[j].rxq.q_id;
				qc[k].type =
					rx_qgrp->splitq.rxq_sets[j].rxq.q_type;
			} else {
				qc[k].start_queue_id =
					rx_qgrp->singleq.rxqs[j].q_id;
				qc[k].type =
					rx_qgrp->singleq.rxqs[j].q_type;
			}
			qc[k].num_queues = 1;
		}
	}
	if (vport->num_rxq != k - (vport->num_txq + vport->num_complq)) {
		err = -EINVAL;
		goto error;
	}

	if (iecm_is_queue_model_split(vport->rxq_model)) {
		for (i = 0; i < vport->num_rxq_grp; i++) {
			struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];
			struct iecm_queue *q;

			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++, k++) {
				q = &rx_qgrp->splitq.bufq_sets[j].bufq;
				qc[k].type = q->q_type;
				qc[k].start_queue_id = q->q_id;
				qc[k].num_queues = 1;
			}
		}
		if (vport->num_bufq != k - (vport->num_txq +
					    vport->num_complq +
					    vport->num_rxq)) {
			err = -EINVAL;
			goto error;
		}
	}

	err = iecm_send_mb_msg(vport->adapter, vc_op, buf_size, (u8 *)eq);
error:
	kfree(eq);
	return err;
}

/**
 * iecm_send_map_unmap_queue_vector_msg - Send virtchnl map or unmap queue
 * vector message
 * @vport: virtual port data structure
 * @map: true for map and false for unmap
 *
 * Send map or unmap queue vector virtchnl message.  Returns 0 on success,
 * negative on failure.
 */
static int
iecm_send_map_unmap_queue_vector_msg(struct iecm_vport *vport, bool map)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_queue_vector_maps *vqvm;
	struct virtchnl_queue_vector *vqv;
	int buf_size, num_q, err = 0;
	int i, j, k = 0;

	num_q = vport->num_txq + vport->num_rxq;

	buf_size = sizeof(struct virtchnl_queue_vector_maps) +
		   (sizeof(struct virtchnl_queue_vector) * (num_q - 1));
	vqvm = kzalloc(buf_size, GFP_KERNEL);
	if (!vqvm)
		return -ENOMEM;

	vqvm->vport_id = vport->vport_id;
	vqvm->num_maps = num_q;
	vqv = vqvm->qv_maps;

	for (i = 0; i < vport->num_txq_grp; i++) {
		struct iecm_txq_group *tx_qgrp = &vport->txq_grps[i];

		for (j = 0; j < tx_qgrp->num_txq; j++, k++) {
			vqv[k].queue_type = tx_qgrp->txqs[j].q_type;
			vqv[k].queue_id = tx_qgrp->txqs[j].q_id;

			if (iecm_is_queue_model_split(vport->txq_model)) {
				vqv[k].vector_id =
					tx_qgrp->complq->q_vector->v_idx;
				vqv[k].itr_idx =
					tx_qgrp->complq->itr.itr_idx;
			} else {
				vqv[k].vector_id =
					tx_qgrp->txqs[j].q_vector->v_idx;
				vqv[k].itr_idx =
					tx_qgrp->txqs[j].itr.itr_idx;
			}
		}
	}

	if (vport->num_txq != k) {
		err = -EINVAL;
		goto error;
	}

	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];
		int num_rxq;

		if (iecm_is_queue_model_split(vport->rxq_model))
			num_rxq = rx_qgrp->splitq.num_rxq_sets;
		else
			num_rxq = rx_qgrp->singleq.num_rxq;

		for (j = 0; j < num_rxq; j++, k++) {
			struct iecm_queue *rxq;

			if (iecm_is_queue_model_split(vport->rxq_model))
				rxq = &rx_qgrp->splitq.rxq_sets[j].rxq;
			else
				rxq = &rx_qgrp->singleq.rxqs[j];

			vqv[k].queue_type = rxq->q_type;
			vqv[k].queue_id = rxq->q_id;
			vqv[k].vector_id = rxq->q_vector->v_idx;
			vqv[k].itr_idx = rxq->itr.itr_idx;
		}
	}

	if (iecm_is_queue_model_split(vport->txq_model)) {
		if (vport->num_rxq != k - vport->num_complq) {
			err = -EINVAL;
			goto error;
		}
	} else {
		if (vport->num_rxq != k - vport->num_txq) {
			err = -EINVAL;
			goto error;
		}
	}

	if (map) {
		err = iecm_send_mb_msg(adapter,
				       VIRTCHNL_OP_MAP_QUEUE_VECTOR,
				       buf_size, (u8 *)vqvm);
		if (!err)
			err = iecm_wait_for_event(adapter, IECM_VC_MAP_IRQ,
						  IECM_VC_MAP_IRQ_ERR);
	} else {
		err = iecm_send_mb_msg(adapter,
				       VIRTCHNL_OP_UNMAP_QUEUE_VECTOR,
				       buf_size, (u8 *)vqvm);
		if (!err)
			err = iecm_wait_for_event(adapter, IECM_VC_UNMAP_IRQ,
						  IECM_VC_UNMAP_IRQ_ERR);
	}
error:
	kfree(vqvm);
	return err;
}

/**
 * iecm_send_enable_queues_msg - send enable queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send enable queues virtchnl message.  Returns 0 on success, negative on
 * failure.
 */
static int iecm_send_enable_queues_msg(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	int err;

	err = iecm_send_ena_dis_queues_msg(vport,
					   VIRTCHNL_OP_ENABLE_QUEUES_V2);
	if (err)
		return err;

	return iecm_wait_for_event(adapter, IECM_VC_ENA_QUEUES,
				   IECM_VC_ENA_QUEUES_ERR);
}

/**
 * iecm_send_disable_queues_msg - send disable queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send disable queues virtchnl message.	Returns 0 on success, negative
 * on failure.
 */
static int iecm_send_disable_queues_msg(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	int err;

	err = iecm_send_ena_dis_queues_msg(vport,
					   VIRTCHNL_OP_DISABLE_QUEUES_V2);
	if (err)
		return err;

	return iecm_wait_for_event(adapter, IECM_VC_DIS_QUEUES,
				   IECM_VC_DIS_QUEUES_ERR);
}

/**
 * iecm_send_delete_queues_msg - send delete queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send delete queues virtchnl message. Return 0 on success, negative on
 * failure.
 */
int iecm_send_delete_queues_msg(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_create_vport *vport_params;
	struct virtchnl_del_ena_dis_queues *eq;
	struct virtchnl_queue_chunks *chunks;
	int buf_size, num_chunks, err;

	if (vport->adapter->config_data.req_qs_chunks) {
		struct virtchnl_add_queues *vc_aq =
			(struct virtchnl_add_queues *)
			vport->adapter->config_data.req_qs_chunks;
		chunks = &vc_aq->chunks;
	} else {
		vport_params = (struct virtchnl_create_vport *)
				vport->adapter->vport_params_recvd[0];
		 chunks = &vport_params->chunks;
	}

	num_chunks = chunks->num_chunks;
	buf_size = sizeof(struct virtchnl_del_ena_dis_queues) +
		   (sizeof(struct virtchnl_queue_chunk) *
		    (num_chunks - 1));

	eq = kzalloc(buf_size, GFP_KERNEL);
	if (!eq)
		return -ENOMEM;

	eq->vport_id = vport->vport_id;
	eq->chunks.num_chunks = num_chunks;

	memcpy(eq->chunks.chunks, chunks->chunks, num_chunks *
	       sizeof(struct virtchnl_queue_chunk));

	err = iecm_send_mb_msg(vport->adapter, VIRTCHNL_OP_DEL_QUEUES,
			       buf_size, (u8 *)eq);
	if (err)
		goto error;

	err = iecm_wait_for_event(adapter, IECM_VC_DEL_QUEUES,
				  IECM_VC_DEL_QUEUES_ERR);
error:
	kfree(eq);
	return err;
}

/**
 * iecm_send_config_queues_msg - Send config queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send config queues virtchnl message. Returns 0 on success, negative on
 * failure.
 */
static int iecm_send_config_queues_msg(struct iecm_vport *vport)
{
	int err;

	err = iecm_send_config_tx_queues_msg(vport);
	if (err)
		return err;

	return iecm_send_config_rx_queues_msg(vport);
}

/**
 * iecm_send_add_queues_msg - Send virtchnl add queues message
 * @vport: Virtual port private data structure
 * @num_tx_q: number of transmit queues
 * @num_complq: number of transmit completion queues
 * @num_rx_q: number of receive queues
 * @num_rx_bufq: number of receive buffer queues
 *
 * Returns 0 on success, negative on failure.
 */
int iecm_send_add_queues_msg(struct iecm_vport *vport, u16 num_tx_q,
			     u16 num_complq, u16 num_rx_q, u16 num_rx_bufq)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_add_queues aq = {0};
	struct virtchnl_add_queues *vc_msg;
	int size, err;

	vc_msg = (struct virtchnl_add_queues *)adapter->vc_msg;

	aq.vport_id = vport->vport_id;
	aq.num_tx_q = num_tx_q;
	aq.num_tx_complq = num_complq;
	aq.num_rx_q = num_rx_q;
	aq.num_rx_bufq = num_rx_bufq;

	err = iecm_send_mb_msg(adapter,
			       VIRTCHNL_OP_ADD_QUEUES,
			       sizeof(struct virtchnl_add_queues), (u8 *)&aq);
	if (err)
		return err;

	err = iecm_wait_for_event(adapter, IECM_VC_ADD_QUEUES,
				  IECM_VC_ADD_QUEUES_ERR);
	if (err)
		return err;

	kfree(adapter->config_data.req_qs_chunks);
	adapter->config_data.req_qs_chunks = NULL;

	/* compare vc_msg num queues with vport num queues */
	if (vc_msg->num_tx_q != num_tx_q ||
	    vc_msg->num_rx_q != num_rx_q ||
	    vc_msg->num_tx_complq != num_complq ||
	    vc_msg->num_rx_bufq != num_rx_bufq) {
		err = -EINVAL;
		goto error;
	}

	size = sizeof(struct virtchnl_add_queues) +
			((vc_msg->chunks.num_chunks - 1) *
			sizeof(struct virtchnl_queue_chunk));
	adapter->config_data.req_qs_chunks =
		kzalloc(size, GFP_KERNEL);
	if (!adapter->config_data.req_qs_chunks) {
		err = -ENOMEM;
		goto error;
	}
	memcpy(adapter->config_data.req_qs_chunks,
	       adapter->vc_msg, size);
error:
	mutex_unlock(&adapter->vc_msg_lock);
	return err;
}

/**
 * iecm_send_get_stats_msg - Send virtchnl get statistics message
 * @vport: vport to get stats for
 *
 * Returns 0 on success, negative on failure.
 */
int iecm_send_get_stats_msg(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_eth_stats *stats;
	struct virtchnl_queue_select vqs;
	int err;

	stats = (struct virtchnl_eth_stats *)adapter->vc_msg;

	/* Don't send get_stats message if one is pending or the
	 * link is down
	 */
	if (test_bit(IECM_VC_GET_STATS, adapter->vc_state) ||
	    adapter->state <= __IECM_DOWN)
		return 0;

	vqs.vsi_id = vport->vport_id;

	err = iecm_send_mb_msg(adapter, VIRTCHNL_OP_GET_STATS,
			       sizeof(vqs), (u8 *)&vqs);

	if (err)
		return err;

	err = iecm_wait_for_event(adapter, IECM_VC_GET_STATS,
				  IECM_VC_GET_STATS_ERR);
	if (err)
		return err;

	vport->netstats.rx_packets = stats->rx_unicast +
				     stats->rx_multicast +
				     stats->rx_broadcast;
	vport->netstats.tx_packets = stats->tx_unicast +
				     stats->tx_multicast +
				     stats->tx_broadcast;
	vport->netstats.rx_bytes = stats->rx_bytes;
	vport->netstats.tx_bytes = stats->tx_bytes;
	vport->netstats.tx_errors = stats->tx_errors;
	vport->netstats.rx_dropped = stats->rx_discards;
	vport->netstats.tx_dropped = stats->tx_discards;
	mutex_unlock(&adapter->vc_msg_lock);

	return 0;
}

/**
 * iecm_send_get_set_rss_hash_msg - Send set or get RSS hash message
 * @vport: virtual port data structure
 * @get: flag to get or set RSS hash
 *
 * Returns 0 on success, negative on failure.
 */
int iecm_send_get_set_rss_hash_msg(struct iecm_vport *vport, bool get)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_rss_hash rh = {0};
	int err;

	rh.vport_id = vport->vport_id;
	rh.hash = adapter->rss_data.rss_hash;

	if (get) {
		err = iecm_send_mb_msg(adapter, VIRTCHNL_OP_GET_RSS_HASH,
				       sizeof(rh), (u8 *)&rh);
		if (err)
			return err;

		err = iecm_wait_for_event(adapter, IECM_VC_GET_RSS_HASH,
					  IECM_VC_GET_RSS_HASH_ERR);
		if (err)
			return err;

		memcpy(&rh, adapter->vc_msg, sizeof(rh));
		adapter->rss_data.rss_hash = rh.hash;
		/* Leave the buffer clean for next message */
		memset(adapter->vc_msg, 0, IECM_DFLT_MBX_BUF_SIZE);
		mutex_unlock(&adapter->vc_msg_lock);

		return 0;
	}

	err = iecm_send_mb_msg(adapter, VIRTCHNL_OP_SET_RSS_HASH,
			       sizeof(rh), (u8 *)&rh);
	if (err)
		return err;

	return  iecm_wait_for_event(adapter, IECM_VC_SET_RSS_HASH,
				  IECM_VC_SET_RSS_HASH_ERR);
}

/**
 * iecm_send_get_set_rss_lut_msg - Send virtchnl get or set RSS lut message
 * @vport: virtual port data structure
 * @get: flag to set or get RSS look up table
 *
 * Returns 0 on success, negative on failure.
 */
int iecm_send_get_set_rss_lut_msg(struct iecm_vport *vport, bool get)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_rss_lut_v2 *recv_rl;
	struct virtchnl_rss_lut_v2 *rl;
	int buf_size, lut_buf_size;
	int i, err = 0;

	buf_size = sizeof(struct virtchnl_rss_lut_v2) +
		       (sizeof(u16) * (adapter->rss_data.rss_lut_size - 1));
	rl = kzalloc(buf_size, GFP_KERNEL);
	if (!rl)
		return -ENOMEM;

	if (!get) {
		rl->lut_entries = adapter->rss_data.rss_lut_size;
		for (i = 0; i < adapter->rss_data.rss_lut_size; i++)
			rl->lut[i] = adapter->rss_data.rss_lut[i];
	}
	rl->vport_id = vport->vport_id;

	if (get) {
		err = iecm_send_mb_msg(vport->adapter, VIRTCHNL_OP_GET_RSS_LUT,
				       buf_size, (u8 *)rl);
		if (err)
			goto error;

		err = iecm_wait_for_event(adapter, IECM_VC_GET_RSS_LUT,
					  IECM_VC_GET_RSS_LUT_ERR);
		if (err)
			goto error;

		recv_rl = (struct virtchnl_rss_lut_v2 *)adapter->vc_msg;
		if (adapter->rss_data.rss_lut_size !=
		    recv_rl->lut_entries) {
			adapter->rss_data.rss_lut_size =
				recv_rl->lut_entries;
			kfree(adapter->rss_data.rss_lut);

			lut_buf_size = adapter->rss_data.rss_lut_size *
					sizeof(u16);
			adapter->rss_data.rss_lut = kzalloc(lut_buf_size,
							    GFP_KERNEL);
			if (!adapter->rss_data.rss_lut) {
				adapter->rss_data.rss_lut_size = 0;
				/* Leave the buffer clean */
				memset(adapter->vc_msg, 0,
				       IECM_DFLT_MBX_BUF_SIZE);
				mutex_unlock(&adapter->vc_msg_lock);
				err = -ENOMEM;
				goto error;
			}
		}
		memcpy(adapter->rss_data.rss_lut, adapter->vc_msg,
		       adapter->rss_data.rss_lut_size);
		/* Leave the buffer clean for next message */
		memset(adapter->vc_msg, 0, IECM_DFLT_MBX_BUF_SIZE);
		mutex_unlock(&adapter->vc_msg_lock);
	} else {
		err = iecm_send_mb_msg(adapter, VIRTCHNL_OP_SET_RSS_LUT,
				       buf_size, (u8 *)rl);
		if (err)
			goto error;

		err = iecm_wait_for_event(adapter, IECM_VC_SET_RSS_LUT,
					  IECM_VC_SET_RSS_LUT_ERR);
	}
error:
	kfree(rl);
	return err;
}

/**
 * iecm_send_get_set_rss_key_msg - Send virtchnl get or set RSS key message
 * @vport: virtual port data structure
 * @get: flag to set or get RSS look up table
 *
 * Returns 0 on success, negative on failure
 */
int iecm_send_get_set_rss_key_msg(struct iecm_vport *vport, bool get)
{
	struct iecm_adapter *adapter = vport->adapter;
	struct virtchnl_rss_key *recv_rk;
	struct virtchnl_rss_key *rk;
	int i, buf_size, err = 0;

	buf_size = sizeof(struct virtchnl_rss_key) +
		   (sizeof(u8) * (adapter->rss_data.rss_key_size - 1));
	rk = kzalloc(buf_size, GFP_KERNEL);
	if (!rk)
		return -ENOMEM;
	rk->vsi_id = vport->vport_id;

	if (get) {
		err = iecm_send_mb_msg(adapter, VIRTCHNL_OP_GET_RSS_KEY,
				       buf_size, (u8 *)rk);
		if (err)
			goto error;

		err = iecm_wait_for_event(adapter, IECM_VC_GET_RSS_KEY,
					  IECM_VC_GET_RSS_KEY_ERR);
		if (err)
			goto error;

		recv_rk = (struct virtchnl_rss_key *)adapter->vc_msg;
		if (adapter->rss_data.rss_key_size !=
		    recv_rk->key_len) {
			adapter->rss_data.rss_key_size =
				min_t(u16, NETDEV_RSS_KEY_LEN,
				      recv_rk->key_len);
			kfree(adapter->rss_data.rss_key);
			adapter->rss_data.rss_key = (u8 *)
				kzalloc(adapter->rss_data.rss_key_size,
					GFP_KERNEL);
			if (!adapter->rss_data.rss_key) {
				adapter->rss_data.rss_key_size = 0;
				/* Leave the buffer clean */
				memset(adapter->vc_msg, 0,
				       IECM_DFLT_MBX_BUF_SIZE);
				mutex_unlock(&adapter->vc_msg_lock);
				err = -ENOMEM;
				goto error;
			}
		}
		memcpy(adapter->rss_data.rss_key, adapter->vc_msg,
		       adapter->rss_data.rss_key_size);
		/* Leave the buffer clean for next message */
		memset(adapter->vc_msg, 0, IECM_DFLT_MBX_BUF_SIZE);
		mutex_unlock(&adapter->vc_msg_lock);
	} else {
		rk->key_len = adapter->rss_data.rss_key_size;
		for (i = 0; i < adapter->rss_data.rss_key_size; i++)
			rk->key[i] = adapter->rss_data.rss_key[i];

		err = iecm_send_mb_msg(adapter, VIRTCHNL_OP_CONFIG_RSS_KEY,
				       buf_size, (u8 *)rk);
		if (err)
			goto error;

		err = iecm_wait_for_event(adapter, IECM_VC_CONFIG_RSS_KEY,
					  IECM_VC_CONFIG_RSS_KEY_ERR);
	}
error:
	kfree(rk);
	return err;
}

/**
 * iecm_send_get_rx_ptype_msg - Send virtchnl get or set RSS key message
 * @vport: virtual port data structure
 *
 * Returns 0 on success, negative on failure.
 */
int iecm_send_get_rx_ptype_msg(struct iecm_vport *vport)
{
	struct iecm_rx_ptype_decoded *rx_ptype_lkup = vport->rx_ptype_lkup;
	static const int ptype_list[IECM_RX_SUPP_PTYPE] = {
		0, 1,
		11, 12,
		22, 23, 24, 25, 26, 27, 28,
		88, 89, 90, 91, 92, 93, 94
	};
	int i;

	for (i = 0; i < IECM_RX_MAX_PTYPE; i++)
		rx_ptype_lkup[i] = iecm_rx_ptype_lkup[0];

	for (i = 0; i < IECM_RX_SUPP_PTYPE; i++) {
		int j = ptype_list[i];

		rx_ptype_lkup[j] = iecm_rx_ptype_lkup[i];
		rx_ptype_lkup[j].ptype = ptype_list[i];
	};

	return 0;
}

/**
 * iecm_find_ctlq - Given a type and id, find ctlq info
 * @hw: hardware struct
 * @type: type of ctrlq to find
 * @id: ctlq id to find
 *
 * Returns pointer to found ctlq info struct, NULL otherwise.
 */
static struct iecm_ctlq_info *iecm_find_ctlq(struct iecm_hw *hw,
					     enum iecm_ctlq_type type, int id)
{
	struct iecm_ctlq_info *cq, *tmp;

	list_for_each_entry_safe(cq, tmp, &hw->cq_list_head, cq_list) {
		if (cq->q_id == id && cq->cq_type == type)
			return cq;
	}

	return NULL;
}

/**
 * iecm_deinit_dflt_mbx - De initialize mailbox
 * @adapter: adapter info struct
 */
void iecm_deinit_dflt_mbx(struct iecm_adapter *adapter)
{
	cancel_delayed_work_sync(&adapter->init_task);
	iecm_ctlq_deinit(&adapter->hw);
}

/**
 * iecm_init_dflt_mbx - Setup default mailbox parameters and make request
 * @adapter: adapter info struct
 *
 * Returns 0 on success, negative otherwise
 */
int iecm_init_dflt_mbx(struct iecm_adapter *adapter)
{
	struct iecm_ctlq_create_info ctlq_info[] = {
		{
			.type = IECM_CTLQ_TYPE_MAILBOX_TX,
			.id = IECM_DFLT_MBX_ID,
			.len = IECM_DFLT_MBX_Q_LEN,
			.buf_size = IECM_DFLT_MBX_BUF_SIZE
		},
		{
			.type = IECM_CTLQ_TYPE_MAILBOX_RX,
			.id = IECM_DFLT_MBX_ID,
			.len = IECM_DFLT_MBX_Q_LEN,
			.buf_size = IECM_DFLT_MBX_BUF_SIZE
		}
	};
	struct iecm_hw *hw = &adapter->hw;
	int err;

	adapter->dev_ops.reg_ops.ctlq_reg_init(ctlq_info);

#define NUM_Q 2
	err = iecm_ctlq_init(hw, NUM_Q, ctlq_info);
	if (err)
		return -EINVAL;

	hw->asq = iecm_find_ctlq(hw, IECM_CTLQ_TYPE_MAILBOX_TX,
				 IECM_DFLT_MBX_ID);
	hw->arq = iecm_find_ctlq(hw, IECM_CTLQ_TYPE_MAILBOX_RX,
				 IECM_DFLT_MBX_ID);

	if (!hw->asq || !hw->arq) {
		iecm_ctlq_deinit(hw);
		return -ENOENT;
	}
	adapter->state = __IECM_STARTUP;
	/* Skew the delay for init tasks for each function based on fn number
	 * to prevent every function from making the same call simultaneously.
	 */
	queue_delayed_work(adapter->init_wq, &adapter->init_task,
			   msecs_to_jiffies(5 * (adapter->pdev->devfn & 0x07)));
	return 0;
}

/**
 * iecm_vport_params_buf_alloc - Allocate memory for mailbox resources
 * @adapter: Driver specific private data structure
 *
 * Will alloc memory to hold the vport parameters received on mailbox
 */
int iecm_vport_params_buf_alloc(struct iecm_adapter *adapter)
{
	adapter->vport_params_reqd = kcalloc(IECM_MAX_NUM_VPORTS,
					     sizeof(*adapter->vport_params_reqd),
					     GFP_KERNEL);
	if (!adapter->vport_params_reqd)
		return -ENOMEM;

	adapter->vport_params_recvd = kcalloc(IECM_MAX_NUM_VPORTS,
					      sizeof(*adapter->vport_params_recvd),
					      GFP_KERNEL);
	if (!adapter->vport_params_recvd) {
		kfree(adapter->vport_params_reqd);
		return -ENOMEM;
	}

	return 0;
}

/**
 * iecm_vport_params_buf_rel - Release memory for mailbox resources
 * @adapter: Driver specific private data structure
 *
 * Will release memory to hold the vport parameters received on mailbox
 */
void iecm_vport_params_buf_rel(struct iecm_adapter *adapter)
{
	int i = 0;

	for (i = 0; i < IECM_MAX_NUM_VPORTS; i++) {
		kfree(adapter->vport_params_recvd[i]);
		kfree(adapter->vport_params_reqd[i]);
	}

	kfree(adapter->caps);
	kfree(adapter->config_data.req_qs_chunks);
}

/**
 * iecm_vc_core_init - Initialize mailbox and get resources
 * @adapter: Driver specific private structure
 * @vport_id: Virtual port identifier
 *
 * Will check if HW is ready with reset complete. Initializes the mailbox and
 * communicate with master to get all the default vport parameters. Returns 0
 * on success, negative on failure.
 */
int iecm_vc_core_init(struct iecm_adapter *adapter, int *vport_id)
{
	switch (adapter->state) {
	case __IECM_STARTUP:
		if (iecm_send_ver_msg(adapter))
			goto init_failed;
		adapter->state = __IECM_VER_CHECK;
		goto restart;
	case __IECM_VER_CHECK:
		if (iecm_recv_ver_msg(adapter))
			goto init_failed;
		adapter->state = __IECM_GET_CAPS;
		if (adapter->dev_ops.vc_ops.get_caps(adapter))
			goto init_failed;
		goto restart;
	case __IECM_GET_CAPS:
		if (iecm_recv_get_caps_msg(adapter))
			goto init_failed;
		if (iecm_send_create_vport_msg(adapter))
			goto init_failed;
		adapter->state = __IECM_GET_DFLT_VPORT_PARAMS;
		goto restart;
	case __IECM_GET_DFLT_VPORT_PARAMS:
		if (iecm_recv_create_vport_msg(adapter, vport_id))
			goto init_failed;
		adapter->state = __IECM_INIT_SW;
		break;
	case __IECM_INIT_SW:
		break;
	default:
		dev_err(&adapter->pdev->dev, "Device is in bad state: %d\n",
			adapter->state);
		goto init_failed;
	}

	return 0;
restart:
	queue_delayed_work(adapter->init_wq, &adapter->init_task,
			   msecs_to_jiffies(30));
	/* Not an error. Using try again to continue with state machine */
	return -EAGAIN;
init_failed:
	if (++adapter->mb_wait_count > IECM_MB_MAX_ERR) {
		dev_err(&adapter->pdev->dev, "Failed to establish mailbox communications with hardware\n");
		return -EFAULT;
	}
	adapter->state = __IECM_STARTUP;
	queue_delayed_work(adapter->init_wq, &adapter->init_task, HZ);
	return -EAGAIN;
}
EXPORT_SYMBOL(iecm_vc_core_init);

/**
 * iecm_vport_init - Initialize virtual port
 * @vport: virtual port to be initialized
 * @vport_id: Unique identification number of vport
 *
 * Will initialize vport with the info received through MB earlier
 */
static void iecm_vport_init(struct iecm_vport *vport,
			    __always_unused int vport_id)
{
	/* stub */
}

/**
 * iecm_vport_get_vec_ids - Initialize vector id from Mailbox parameters
 * @vecids: Array of vector ids
 * @num_vecids: number of vector ids
 * @chunks: vector ids received over mailbox
 *
 * Will initialize all vector ids with ids received as mailbox parameters
 * Returns number of ids filled
 */
int
iecm_vport_get_vec_ids(u16 *vecids, int num_vecids,
		       struct virtchnl_vector_chunks *chunks)
{
	/* stub */
}

/**
 * iecm_vport_get_queue_ids - Initialize queue id from Mailbox parameters
 * @qids: Array of queue ids
 * @num_qids: number of queue ids
 * @q_type: queue model
 * @chunks: queue ids received over mailbox
 *
 * Will initialize all queue ids with ids received as mailbox parameters
 * Returns number of ids filled
 */
static int
iecm_vport_get_queue_ids(u16 *qids, int num_qids,
			 enum virtchnl_queue_type q_type,
			 struct virtchnl_queue_chunks *chunks)
{
	int num_chunks = chunks->num_chunks;
	struct virtchnl_queue_chunk *chunk;
	int num_q_id_filled = 0;
	int start_q_id;
	int num_q;
	int i;

	while (num_chunks) {
		chunk = &chunks->chunks[num_chunks - 1];
		if (chunk->type == q_type) {
			num_q = chunk->num_queues;
			start_q_id = chunk->start_queue_id;
			for (i = 0; i < num_q; i++) {
				if ((num_q_id_filled + i) < num_qids) {
					qids[num_q_id_filled + i] = start_q_id;
					start_q_id++;
				} else {
					break;
				}
			}
			num_q_id_filled = num_q_id_filled + i;
		}
		num_chunks--;
	}

	return num_q_id_filled;
}

/**
 * __iecm_vport_queue_ids_init - Initialize queue ids from Mailbox parameters
 * @vport: virtual port for which the queues ids are initialized
 * @qids: queue ids
 * @num_qids: number of queue ids
 * @q_type: type of queue
 *
 * Will initialize all queue ids with ids received as mailbox
 * parameters. Returns number of queue ids initialized.
 */
static int
__iecm_vport_queue_ids_init(struct iecm_vport *vport, u16 *qids,
			    int num_qids, enum virtchnl_queue_type q_type)
{
	struct iecm_queue *q;
	int i, j, k = 0;

	switch (q_type) {
	case VIRTCHNL_QUEUE_TYPE_TX:
		for (i = 0; i < vport->num_txq_grp; i++) {
			struct iecm_txq_group *tx_qgrp = &vport->txq_grps[i];

			for (j = 0; j < tx_qgrp->num_txq; j++) {
				if (k < num_qids) {
					tx_qgrp->txqs[j].q_id = qids[k];
					tx_qgrp->txqs[j].q_type =
						VIRTCHNL_QUEUE_TYPE_TX;
					k++;
				} else {
					break;
				}
			}
		}
		break;
	case VIRTCHNL_QUEUE_TYPE_RX:
		for (i = 0; i < vport->num_rxq_grp; i++) {
			struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];
			int num_rxq;

			if (iecm_is_queue_model_split(vport->rxq_model))
				num_rxq = rx_qgrp->splitq.num_rxq_sets;
			else
				num_rxq = rx_qgrp->singleq.num_rxq;

			for (j = 0; j < num_rxq && k < num_qids; j++, k++) {
				if (iecm_is_queue_model_split(vport->rxq_model))
					q = &rx_qgrp->splitq.rxq_sets[j].rxq;
				else
					q = &rx_qgrp->singleq.rxqs[j];
				q->q_id = qids[k];
				q->q_type = VIRTCHNL_QUEUE_TYPE_RX;
			}
		}
		break;
	case VIRTCHNL_QUEUE_TYPE_TX_COMPLETION:
		for (i = 0; i < vport->num_txq_grp; i++) {
			struct iecm_txq_group *tx_qgrp = &vport->txq_grps[i];

			if (k < num_qids) {
				tx_qgrp->complq->q_id = qids[k];
				tx_qgrp->complq->q_type =
					VIRTCHNL_QUEUE_TYPE_TX_COMPLETION;
				k++;
			} else {
				break;
			}
		}
		break;
	case VIRTCHNL_QUEUE_TYPE_RX_BUFFER:
		for (i = 0; i < vport->num_rxq_grp; i++) {
			struct iecm_rxq_group *rx_qgrp = &vport->rxq_grps[i];

			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++) {
				if (k < num_qids) {
					q = &rx_qgrp->splitq.bufq_sets[j].bufq;
					q->q_id = qids[k];
					q->q_type =
						VIRTCHNL_QUEUE_TYPE_RX_BUFFER;
					k++;
				} else {
					break;
				}
			}
		}
		break;
	}

	return k;
}

/**
 * iecm_vport_queue_ids_init - Initialize queue ids from Mailbox parameters
 * @vport: virtual port for which the queues ids are initialized
 *
 * Will initialize all queue ids with ids received as mailbox parameters.
 * Returns 0 on success, negative if all the queues are not initialized.
 */
static int iecm_vport_queue_ids_init(struct iecm_vport *vport)
{
	struct virtchnl_create_vport *vport_params;
	struct virtchnl_queue_chunks *chunks;
	enum virtchnl_queue_type q_type;
	/* We may never deal with more that 256 same type of queues */
#define IECM_MAX_QIDS	256
	u16 qids[IECM_MAX_QIDS];
	int num_ids;

	if (vport->adapter->config_data.num_req_tx_qs ||
	    vport->adapter->config_data.num_req_rx_qs) {
		struct virtchnl_add_queues *vc_aq =
			(struct virtchnl_add_queues *)
			vport->adapter->config_data.req_qs_chunks;
		chunks = &vc_aq->chunks;
	} else {
		vport_params = (struct virtchnl_create_vport *)
				vport->adapter->vport_params_recvd[0];
		chunks = &vport_params->chunks;
		/* compare vport_params num queues with vport num queues */
		if (vport_params->num_tx_q != vport->num_txq ||
		    vport_params->num_rx_q != vport->num_rxq ||
		    vport_params->num_tx_complq != vport->num_complq ||
		    vport_params->num_rx_bufq != vport->num_bufq)
			return -EINVAL;
	}

	num_ids = iecm_vport_get_queue_ids(qids, IECM_MAX_QIDS,
					   VIRTCHNL_QUEUE_TYPE_TX,
					   chunks);
	if (num_ids != vport->num_txq)
		return -EINVAL;
	num_ids = __iecm_vport_queue_ids_init(vport, qids, num_ids,
					      VIRTCHNL_QUEUE_TYPE_TX);
	if (num_ids != vport->num_txq)
		return -EINVAL;
	num_ids = iecm_vport_get_queue_ids(qids, IECM_MAX_QIDS,
					   VIRTCHNL_QUEUE_TYPE_RX,
					   chunks);
	if (num_ids != vport->num_rxq)
		return -EINVAL;
	num_ids = __iecm_vport_queue_ids_init(vport, qids, num_ids,
					      VIRTCHNL_QUEUE_TYPE_RX);
	if (num_ids != vport->num_rxq)
		return -EINVAL;

	if (iecm_is_queue_model_split(vport->txq_model)) {
		q_type = VIRTCHNL_QUEUE_TYPE_TX_COMPLETION;
		num_ids = iecm_vport_get_queue_ids(qids, IECM_MAX_QIDS, q_type,
						   chunks);
		if (num_ids != vport->num_complq)
			return -EINVAL;
		num_ids = __iecm_vport_queue_ids_init(vport, qids,
						      num_ids,
						      q_type);
		if (num_ids != vport->num_complq)
			return -EINVAL;
	}

	if (iecm_is_queue_model_split(vport->rxq_model)) {
		q_type = VIRTCHNL_QUEUE_TYPE_RX_BUFFER;
		num_ids = iecm_vport_get_queue_ids(qids, IECM_MAX_QIDS, q_type,
						   chunks);
		if (num_ids != vport->num_bufq)
			return -EINVAL;
		num_ids = __iecm_vport_queue_ids_init(vport, qids, num_ids,
						      q_type);
		if (num_ids != vport->num_bufq)
			return -EINVAL;
	}

	return 0;
}

/**
 * iecm_vport_adjust_qs - Adjust to new requested queues
 * @vport: virtual port data struct
 *
 * Renegotiate queues.	 Returns 0 on success, negative on failure.
 */
void iecm_vport_adjust_qs(struct iecm_vport *vport)
{
	struct virtchnl_create_vport vport_msg;

	vport_msg.txq_model = vport->txq_model;
	vport_msg.rxq_model = vport->rxq_model;
	iecm_vport_calc_total_qs(vport->adapter, &vport_msg);

	iecm_vport_init_num_qs(vport, &vport_msg);
	iecm_vport_calc_num_q_groups(vport);
	iecm_vport_calc_num_q_vec(vport);
}

/**
 * iecm_is_capability_ena - Default implementation of capability checking
 * @adapter: Private data struct
 * @flag: flag to check
 *
 * Return true if capability is supported, false otherwise
 */
static bool iecm_is_capability_ena(struct iecm_adapter *adapter, u64 flag)
{
	return ((struct virtchnl_get_capabilities *)adapter->caps)->cap_flags &
	       flag;
}

/**
 * iecm_vc_ops_init - Initialize virtchnl common API
 * @adapter: Driver specific private structure
 *
 * Initialize the function pointers with the extended feature set functions
 * as APF will deal only with new set of opcodes.
 */
void iecm_vc_ops_init(struct iecm_adapter *adapter)
{
	struct iecm_virtchnl_ops *vc_ops = &adapter->dev_ops.vc_ops;

	vc_ops->core_init = iecm_vc_core_init;
	vc_ops->vport_init = iecm_vport_init;
	vc_ops->vport_queue_ids_init = iecm_vport_queue_ids_init;
	vc_ops->get_caps = iecm_send_get_caps_msg;
	vc_ops->is_cap_ena = iecm_is_capability_ena;
	vc_ops->config_queues = iecm_send_config_queues_msg;
	vc_ops->enable_queues = iecm_send_enable_queues_msg;
	vc_ops->disable_queues = iecm_send_disable_queues_msg;
	vc_ops->irq_map_unmap = iecm_send_map_unmap_queue_vector_msg;
	vc_ops->enable_vport = iecm_send_enable_vport_msg;
	vc_ops->disable_vport = iecm_send_disable_vport_msg;
	vc_ops->destroy_vport = iecm_send_destroy_vport_msg;
	vc_ops->get_ptype = iecm_send_get_rx_ptype_msg;
	vc_ops->get_set_rss_lut = iecm_send_get_set_rss_lut_msg;
	vc_ops->get_set_rss_hash = iecm_send_get_set_rss_hash_msg;
	vc_ops->adjust_qs = iecm_vport_adjust_qs;
	vc_ops->recv_mbx_msg = NULL;
}
EXPORT_SYMBOL(iecm_vc_ops_init);
