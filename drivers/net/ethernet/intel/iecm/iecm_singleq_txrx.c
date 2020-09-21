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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_tx_singleq_clean - Reclaim resources from queue
 * @tx_q: Tx queue to clean
 * @napi_budget: Used to determine if we are in netpoll
 *
 */
static bool iecm_tx_singleq_clean(struct iecm_queue *tx_q, int napi_budget)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_rx_bump_ntc - Bump and wrap q->next_to_clean value
 * @q: queue to bump
 */
static void iecm_singleq_rx_bump_ntc(struct iecm_queue *q)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_singleq_napi_poll - NAPI handler
 * @napi: struct from which you get q_vector
 * @budget: budget provided by stack
 */
int iecm_vport_singleq_napi_poll(struct napi_struct *napi, int budget)
{
	/* stub */
}
