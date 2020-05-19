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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_deinit_dflt_mbx - De initialize mailbox
 * @adapter: adapter info struct
 */
void iecm_deinit_dflt_mbx(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_init_dflt_mbx - Setup default mailbox parameters and make request
 * @adapter: adapter info struct
 *
 * Returns 0 on success, negative otherwise
 */
enum iecm_status iecm_init_dflt_mbx(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_vport_params_buf_alloc - Allocate memory for MailBox resources
 * @adapter: Driver specific private data structure
 *
 * Will alloc memory to hold the vport parameters received on MailBox
 */
int iecm_vport_params_buf_alloc(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_vport_params_buf_rel - Release memory for MailBox resources
 * @adapter: Driver specific private data structure
 *
 * Will release memory to hold the vport parameters received on MailBox
 */
void iecm_vport_params_buf_rel(struct iecm_adapter *adapter)
{
	/* stub */
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
	/* stub */
}
EXPORT_SYMBOL(iecm_vc_ops_init);
