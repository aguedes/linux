// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Intel Corporation */

#include "iecm.h"

/**
 * iecm_recv_event_msg - Receive virtchnl event message
 * @vport: virtual port structure
 *
 * Receive virtchnl event message
 */
void iecm_recv_event_msg(struct iecm_vport *vport)
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
 * Returns success or failure
 */
enum iecm_status
iecm_mb_clean(struct iecm_adapter *adapter)
{
	enum iecm_status err = 0;
	u16 num_q_msg = IECM_DFLT_MBX_Q_LEN;
	struct iecm_ctlq_msg **q_msg;
	struct iecm_dma_mem *dma_mem;
	u16 i;

	q_msg = kcalloc(num_q_msg, sizeof(struct iecm_ctlq_msg *), GFP_KERNEL);
	if (!q_msg) {
		err = IECM_ERR_NO_MEMORY;
		goto error;
	}

	err = iecm_ctlq_clean_sq(&adapter->hw, adapter->hw.asq, &num_q_msg,
				 q_msg);
	if (err)
		goto error;

	for (i = 0; i < num_q_msg; i++) {
		dma_mem = q_msg[i]->ctx.indirect.payload;
		if (dma_mem)
			dmam_free_coherent(&adapter->pdev->dev, dma_mem->size,
					   dma_mem->va, dma_mem->pa);
		kfree(q_msg[i]);
	}
	kfree(q_msg);

error:
	return err;
}

/**
 * iecm_send_mb_msg - Send message over mailbox
 * @adapter: Driver specific private structure
 * @op: virtchnl opcode
 * @msg_size: size of the payload
 * @msg: pointer to buffer holding the payload
 *
 * Will prepare the control queue message and initiates the send api
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_mb_msg(struct iecm_adapter *adapter, enum virtchnl_ops op,
		 u16 msg_size, u8 *msg)
{
	enum iecm_status err = 0;
	struct iecm_ctlq_msg *ctlq_msg;
	struct iecm_dma_mem *dma_mem;

	err = iecm_mb_clean(adapter);
	if (err)
		goto err;

	ctlq_msg = kzalloc(sizeof(struct iecm_ctlq_msg), GFP_KERNEL);
	if (!ctlq_msg) {
		err = IECM_ERR_NO_MEMORY;
		goto err;
	}

	dma_mem = kzalloc(sizeof(struct iecm_dma_mem), GFP_KERNEL);
	if (!dma_mem) {
		err = IECM_ERR_NO_MEMORY;
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
		err = IECM_ERR_NO_MEMORY;
		goto dma_alloc_error;
	}
	memcpy(dma_mem->va, msg, msg_size);
	ctlq_msg->ctx.indirect.payload = dma_mem;

	err = iecm_ctlq_send(&adapter->hw, adapter->hw.asq, 1, ctlq_msg);
	if (err)
		goto send_error;

	return err;
send_error:
	dmam_free_coherent(&adapter->pdev->dev, dma_mem->size, dma_mem->va,
			   dma_mem->pa);
dma_alloc_error:
	kfree(dma_mem);
dma_mem_error:
	kfree(ctlq_msg);
err:
	return err;
}
EXPORT_SYMBOL(iecm_send_mb_msg);

/**
 * iecm_recv_mb_msg - Receive message over mailbox
 * @adapter: Driver specific private structure
 * @op: virtchannel operation code
 * @msg: Received message holding buffer
 * @msg_size: message size
 *
 * Will receive control queue message and posts the receive buffer
 */
enum iecm_status
iecm_recv_mb_msg(struct iecm_adapter *adapter, enum virtchnl_ops op,
		 void *msg, int msg_size)
{
	enum iecm_status status = 0;
	struct iecm_ctlq_msg ctlq_msg;
	struct iecm_dma_mem *dma_mem;
	struct iecm_vport *vport;
	bool work_done = false;
	int payload_size = 0;
	int num_retry = 10;
	u16 num_q_msg;

	vport = adapter->vports[0];
	while (1) {
		/* Try to get one message */
		num_q_msg = 1;
		dma_mem = NULL;
		status = iecm_ctlq_recv(&adapter->hw, adapter->hw.arq,
					&num_q_msg, &ctlq_msg);
		/* If no message then decide if we have to retry based on
		 * opcode
		 */
		if (status || !num_q_msg) {
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
				status =
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

		status = iecm_ctlq_post_rx_buffs(&adapter->hw, adapter->hw.arq,
						 &num_q_msg,
						 &dma_mem);
		/* If post failed clear the only buffer we supplied */
		if (status && dma_mem)
			dmam_free_coherent(&adapter->pdev->dev, dma_mem->size,
					   dma_mem->va, dma_mem->pa);
		/* Applies only if we are looking for a specific opcode */
		if (work_done)
			break;
	}

	return status;
}
EXPORT_SYMBOL(iecm_recv_mb_msg);

/**
 * iecm_send_ver_msg - send virtchnl version message
 * @adapter: Driver specific private structure
 *
 * Send virtchnl version message
 */
static enum iecm_status
iecm_send_ver_msg(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_recv_ver_msg - Receive virtchnl version message
 * @adapter: Driver specific private structure
 *
 * Receive virtchnl version message
 */
static enum iecm_status
iecm_recv_ver_msg(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_send_get_caps_msg - Send virtchnl get capabilities message
 * @adapter: Driver specific private structure
 *
 * send virtchl get capabilities message
 */
enum iecm_status
iecm_send_get_caps_msg(struct iecm_adapter *adapter)
{
	/* stub */
}
EXPORT_SYMBOL(iecm_send_get_caps_msg);

/**
 * iecm_recv_get_caps_msg - Receive virtchnl get capabilities message
 * @adapter: Driver specific private structure
 *
 * Receive virtchnl get capabilities message
 */
static enum iecm_status
iecm_recv_get_caps_msg(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_send_create_vport_msg - Send virtchnl create vport message
 * @adapter: Driver specific private structure
 *
 * send virtchnl creae vport message
 *
 * Returns success or failure
 */
static enum iecm_status
iecm_send_create_vport_msg(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_recv_create_vport_msg - Receive virtchnl create vport message
 * @adapter: Driver specific private structure
 * @vport_id: Virtual port identifier
 *
 * Receive virtchnl create vport message
 *
 * Returns success or failure
 */
static enum iecm_status
iecm_recv_create_vport_msg(struct iecm_adapter *adapter,
			   int *vport_id)
{
	/* stub */
}

/**
 * iecm_wait_for_event - wait for virtchannel response
 * @adapter: Driver private data structure
 * @state: check on state upon timeout after 500ms
 * @err_check: check if this specific error bit is set
 *
 * checks if state is set upon expiry of timeout
 *
 * Returns success or failure
 */
enum iecm_status
iecm_wait_for_event(struct iecm_adapter *adapter,
		    enum iecm_vport_vc_state state,
		    enum iecm_vport_vc_state err_check)
{
	enum iecm_status status;
	int event;

	event = wait_event_timeout(adapter->vchnl_wq,
				   test_and_clear_bit(state, adapter->vc_state),
				   msecs_to_jiffies(500));
	if (event) {
		if (test_and_clear_bit(err_check, adapter->vc_state)) {
			dev_err(&adapter->pdev->dev,
				"VC response error %d", err_check);
			status = IECM_ERR_CFG;
		} else {
			status = 0;
		}
	} else {
		/* Timeout occurred */
		status = IECM_ERR_NOT_READY;
		dev_err(&adapter->pdev->dev, "VC timeout, state = %u", state);
	}

	return status;
}
EXPORT_SYMBOL(iecm_wait_for_event);

/**
 * iecm_send_destroy_vport_msg - Send virtchnl destroy vport message
 * @vport: virtual port data structure
 *
 * send virtchnl destroy vport message
 */
enum iecm_status
iecm_send_destroy_vport_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_enable_vport_msg - Send virtchnl enable vport message
 * @vport: virtual port data structure
 *
 * send enable vport virtchnl message
 */
enum iecm_status
iecm_send_enable_vport_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_disable_vport_msg - Send virtchnl disable vport message
 * @vport: virtual port data structure
 *
 * send disable vport virtchnl message
 */
enum iecm_status
iecm_send_disable_vport_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_config_tx_queues_msg - Send virtchnl config tx queues message
 * @vport: virtual port data structure
 *
 * send config tx queues virtchnl message
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_config_tx_queues_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_config_rx_queues_msg - Send virtchnl config rx queues message
 * @vport: virtual port data structure
 *
 * send config rx queues virtchnl message
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_config_rx_queues_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_ena_dis_queues_msg - Send virtchnl enable or disable
 * queues message
 * @vport: virtual port data structure
 * @vc_op: virtchnl op code to send
 *
 * send enable or disable queues virtchnl message
 *
 * Returns success or failure
 */
static enum iecm_status
iecm_send_ena_dis_queues_msg(struct iecm_vport *vport,
			     enum virtchnl_ops vc_op)
{
	/* stub */
}

/**
 * iecm_send_map_unmap_queue_vector_msg - Send virtchnl map or unmap queue
 * vector message
 * @vport: virtual port data structure
 * @map: true for map and false for unmap
 *
 * send map or unmap queue vector virtchnl message
 *
 * Returns success or failure
 */
static enum iecm_status
iecm_send_map_unmap_queue_vector_msg(struct iecm_vport *vport,
				     bool map)
{
	/* stub */
}

/**
 * iecm_send_enable_queues_msg - send enable queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send enable queues virtchnl message
 */
static enum iecm_status
iecm_send_enable_queues_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_disable_queues_msg - send disable queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send disable queues virtchnl message
 */
static enum iecm_status
iecm_send_disable_queues_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_delete_queues_msg - send delete queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send delete queues virtchnl message
 */
enum iecm_status
iecm_send_delete_queues_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_config_queues_msg - Send config queues virtchnl message
 * @vport: Virtual port private data structure
 *
 * Will send config queues virtchnl message
 */
static enum iecm_status
iecm_send_config_queues_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_add_queues_msg - Send virtchnl add queues message
 * @vport: Virtual port private data structure
 * @num_tx_q: number of transmit queues
 * @num_complq: number of transmit completion queues
 * @num_rx_q: number of receive queues
 * @num_rx_bufq: number of receive buffer queues
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_add_queues_msg(struct iecm_vport *vport, u16 num_tx_q,
			 u16 num_complq, u16 num_rx_q, u16 num_rx_bufq)
{
	/* stub */
}

/**
 * iecm_send_get_stats_msg - Send virtchnl get statistics message
 * @adapter: Driver specific private structure
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_get_stats_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_send_get_set_rss_hash_msg - Send set or get rss hash message
 * @vport: virtual port data structure
 * @get: flag to get or set rss hash
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_get_set_rss_hash_msg(struct iecm_vport *vport, bool get)
{
	/* stub */
}

/**
 * iecm_send_get_set_rss_lut_msg - Send virtchnl get or set rss lut message
 * @vport: virtual port data structure
 * @get: flag to set or get rss look up table
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_get_set_rss_lut_msg(struct iecm_vport *vport, bool get)
{
	/* stub */
}

/**
 * iecm_send_get_set_rss_key_msg - Send virtchnl get or set rss key message
 * @vport: virtual port data structure
 * @get: flag to set or get rss look up table
 *
 * Returns success or failure
 */
enum iecm_status
iecm_send_get_set_rss_key_msg(struct iecm_vport *vport, bool get)
{
	/* stub */
}

/**
 * iecm_send_get_rx_ptype_msg - Send virtchnl get or set rss key message
 * @vport: virtual port data structure
 *
 * Returns success or failure
 */
enum iecm_status iecm_send_get_rx_ptype_msg(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_find_ctlq - Given a type and id, find ctlq info
 * @adapter: adapter info struct
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
enum iecm_status iecm_init_dflt_mbx(struct iecm_adapter *adapter)
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
	enum iecm_status ret;

	adapter->dev_ops.reg_ops.ctlq_reg_init(ctlq_info);

#define NUM_Q 2
	ret = iecm_ctlq_init(hw, NUM_Q, ctlq_info);
	if (ret)
		goto init_mbx_done;

	hw->asq = iecm_find_ctlq(hw, IECM_CTLQ_TYPE_MAILBOX_TX,
				 IECM_DFLT_MBX_ID);
	hw->arq = iecm_find_ctlq(hw, IECM_CTLQ_TYPE_MAILBOX_RX,
				 IECM_DFLT_MBX_ID);

	if (!hw->asq || !hw->arq) {
		iecm_ctlq_deinit(hw);
		ret = IECM_ERR_CTLQ_ERROR;
	}
	adapter->state = __IECM_STARTUP;
	/* Skew the delay for init tasks for each function based on fn number
	 * to prevent every function from making the same call simulatenously.
	 */
	queue_delayed_work(adapter->init_wq, &adapter->init_task,
			   msecs_to_jiffies(5 * (adapter->pdev->devfn & 0x07)));
init_mbx_done:
	return ret;
}

/**
 * iecm_vport_params_buf_alloc - Allocate memory for MailBox resources
 * @adapter: Driver specific private data structure
 *
 * Will alloc memory to hold the vport parameters received on MailBox
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
 * iecm_vport_params_buf_rel - Release memory for MailBox resources
 * @adapter: Driver specific private data structure
 *
 * Will release memory to hold the vport parameters received on MailBox
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
 * communicate with master to get all the default vport parameters.
 */
int iecm_vc_core_init(struct iecm_adapter *adapter, int *vport_id)
{
	/* stub */
}
EXPORT_SYMBOL(iecm_vc_core_init);

/**
 * iecm_vport_init - Initialize virtual port
 * @vport: virtual port to be initialized
 * @vport_id: Unique identification number of vport
 *
 * Will initialize vport with the info received through MB earlier
 */
static void iecm_vport_init(struct iecm_vport *vport, int vport_id)
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_queue_ids_init - Initialize queue ids from Mailbox parameters
 * @vport: virtual port for which the queues ids are initialized
 *
 * Will initialize all queue ids with ids received as mailbox
 * parameters. Returns error if all the queues are not initialized
 */
static
enum iecm_status iecm_vport_queue_ids_init(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_adjust_qs - Adjust to new requested queues
 * @vport: virtual port data struct
 *
 * Renegotiate queues
 */
enum iecm_status iecm_vport_adjust_qs(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vc_ops_init - Initialize virtchnl common api
 * @adapter: Driver specific private structure
 *
 * Initialize the function pointers with the extended feature set functions
 * as APF will deal only with new set of opcodes.
 */
void iecm_vc_ops_init(struct iecm_adapter *adapter)
{
	adapter->dev_ops.vc_ops.core_init = iecm_vc_core_init;
	adapter->dev_ops.vc_ops.vport_init = iecm_vport_init;
	adapter->dev_ops.vc_ops.vport_queue_ids_init =
		iecm_vport_queue_ids_init;
	adapter->dev_ops.vc_ops.get_caps = iecm_send_get_caps_msg;
	adapter->dev_ops.vc_ops.is_cap_ena = iecm_is_capability_ena;
	adapter->dev_ops.vc_ops.config_queues = iecm_send_config_queues_msg;
	adapter->dev_ops.vc_ops.enable_queues = iecm_send_enable_queues_msg;
	adapter->dev_ops.vc_ops.disable_queues = iecm_send_disable_queues_msg;
	adapter->dev_ops.vc_ops.irq_map_unmap =
		iecm_send_map_unmap_queue_vector_msg;
	adapter->dev_ops.vc_ops.enable_vport = iecm_send_enable_vport_msg;
	adapter->dev_ops.vc_ops.disable_vport = iecm_send_disable_vport_msg;
	adapter->dev_ops.vc_ops.destroy_vport = iecm_send_destroy_vport_msg;
	adapter->dev_ops.vc_ops.get_ptype = iecm_send_get_rx_ptype_msg;
	adapter->dev_ops.vc_ops.get_set_rss_lut = iecm_send_get_set_rss_lut_msg;
	adapter->dev_ops.vc_ops.get_set_rss_hash =
		iecm_send_get_set_rss_hash_msg;
	adapter->dev_ops.vc_ops.adjust_qs = iecm_vport_adjust_qs;
	adapter->dev_ops.vc_ops.recv_mbx_msg = NULL;
}
EXPORT_SYMBOL(iecm_vc_ops_init);
