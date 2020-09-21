// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020, Intel Corporation. */

#include <linux/net/intel/iecm_controlq.h>

/**
 * iecm_ctlq_setup_regs - initialize control queue registers
 * @cq: pointer to the specific control queue
 * @q_create_info: structs containing info for each queue to be initialized
 */
static void
iecm_ctlq_setup_regs(struct iecm_ctlq_info *cq,
		     struct iecm_ctlq_create_info *q_create_info)
{
	/* stub */
}

/**
 * iecm_ctlq_init_regs - Initialize control queue registers
 * @hw: pointer to hw struct
 * @cq: pointer to the specific Control queue
 * @is_rxq: true if receive control queue, false otherwise
 *
 * Initialize registers. The caller is expected to have already initialized the
 * descriptor ring memory and buffer memory
 */
static void iecm_ctlq_init_regs(struct iecm_hw *hw, struct iecm_ctlq_info *cq,
				bool is_rxq)
{
	/* stub */
}

/**
 * iecm_ctlq_init_rxq_bufs - populate receive queue descriptors with buf
 * @cq: pointer to the specific Control queue
 *
 * Record the address of the receive queue DMA buffers in the descriptors.
 * The buffers must have been previously allocated.
 */
static void iecm_ctlq_init_rxq_bufs(struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_shutdown - shutdown the CQ
 * @hw: pointer to hw struct
 * @cq: pointer to the specific Control queue
 *
 * The main shutdown routine for any controq queue
 */
static void iecm_ctlq_shutdown(struct iecm_hw *hw, struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_add - add one control queue
 * @hw: pointer to hardware struct
 * @qinfo: info for queue to be created
 * @cq_out: (output) double pointer to control queue to be created
 *
 * Allocate and initialize a control queue and add it to the control queue list.
 * The cq parameter will be allocated/initialized and passed back to the caller
 * if no errors occur.
 *
 * Note: iecm_ctlq_init must be called prior to any calls to iecm_ctlq_add
 */
int iecm_ctlq_add(struct iecm_hw *hw,
		  struct iecm_ctlq_create_info *qinfo,
		  struct iecm_ctlq_info **cq_out)
{
	/* stub */
}

/**
 * iecm_ctlq_remove - deallocate and remove specified control queue
 * @hw: pointer to hardware struct
 * @cq: pointer to control queue to be removed
 */
void iecm_ctlq_remove(struct iecm_hw *hw,
		      struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_init - main initialization routine for all control queues
 * @hw: pointer to hardware struct
 * @num_q: number of queues to initialize
 * @q_info: array of structs containing info for each queue to be initialized
 *
 * This initializes any number and any type of control queues. This is an all
 * or nothing routine; if one fails, all previously allocated queues will be
 * destroyed. This must be called prior to using the individual add/remove
 * APIs.
 */
int iecm_ctlq_init(struct iecm_hw *hw, u8 num_q,
		   struct iecm_ctlq_create_info *q_info)
{
	/* stub */
}

/**
 * iecm_ctlq_deinit - destroy all control queues
 * @hw: pointer to hw struct
 */
int iecm_ctlq_deinit(struct iecm_hw *hw)
{
	/* stub */
}

/**
 * iecm_ctlq_send - send command to Control Queue (CTQ)
 * @hw: pointer to hw struct
 * @cq: handle to control queue struct to send on
 * @num_q_msg: number of messages to send on control queue
 * @q_msg: pointer to array of queue messages to be sent
 *
 * The caller is expected to allocate DMAable buffers and pass them to the
 * send routine via the q_msg struct / control queue specific data struct.
 * The control queue will hold a reference to each send message until
 * the completion for that message has been cleaned.
 */
int iecm_ctlq_send(struct iecm_hw *hw,
		   struct iecm_ctlq_info *cq,
		   u16 num_q_msg,
		   struct iecm_ctlq_msg q_msg[])
{
	/* stub */
}

/**
 * iecm_ctlq_clean_sq - reclaim send descriptors on HW write back for the
 * requested queue
 * @cq: pointer to the specific Control queue
 * @clean_count: (input|output) number of descriptors to clean as input, and
 * number of descriptors actually cleaned as output
 * @msg_status: (output) pointer to msg pointer array to be populated; needs
 * to be allocated by caller
 *
 * Returns an array of message pointers associated with the cleaned
 * descriptors. The pointers are to the original ctlq_msgs sent on the cleaned
 * descriptors.  The status will be returned for each; any messages that failed
 * to send will have a non-zero status. The caller is expected to free original
 * ctlq_msgs and free or reuse the DMA buffers.
 */
int iecm_ctlq_clean_sq(struct iecm_ctlq_info *cq, u16 *clean_count,
		       struct iecm_ctlq_msg *msg_status[])
{
	/* stub */
}

/**
 * iecm_ctlq_post_rx_buffs - post buffers to descriptor ring
 * @hw: pointer to hw struct
 * @cq: pointer to control queue handle
 * @buff_count: (input|output) input is number of buffers caller is trying to
 * return; output is number of buffers that were not posted
 * @buffs: array of pointers to DMA mem structs to be given to hardware
 *
 * Caller uses this function to return DMA buffers to the descriptor ring after
 * consuming them; buff_count will be the number of buffers.
 *
 * Note: this function needs to be called after a receive call even
 * if there are no DMA buffers to be returned, i.e. buff_count = 0,
 * buffs = NULL to support direct commands
 */
int iecm_ctlq_post_rx_buffs(struct iecm_hw *hw, struct iecm_ctlq_info *cq,
			    u16 *buff_count, struct iecm_dma_mem **buffs)
{
	/* stub */
}

/**
 * iecm_ctlq_recv - receive control queue message call back
 * @cq: pointer to control queue handle to receive on
 * @num_q_msg: (input|output) input number of messages that should be received;
 * output number of messages actually received
 * @q_msg: (output) array of received control queue messages on this q;
 * needs to be pre-allocated by caller for as many messages as requested
 *
 * Called by interrupt handler or polling mechanism. Caller is expected
 * to free buffers
 */
int iecm_ctlq_recv(struct iecm_ctlq_info *cq, u16 *num_q_msg,
		   struct iecm_ctlq_msg *q_msg)
{
	/* stub */
}
