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
	/* stub */
}

/**
 * iecm_buf_lifo_pop - pop a buffer pointer from stack
 * @stack: pointer to stack struct
 **/
static struct iecm_tx_buf *iecm_buf_lifo_pop(struct iecm_buf_lifo *stack)
{
	/* stub */
}

/**
 * iecm_get_stats64 - get statistics for network device structure
 * @netdev: network interface device structure
 * @stats: main device statistics structure
 */
void iecm_get_stats64(struct net_device *netdev,
		      struct rtnl_link_stats64 *stats)
{
	/* stub */
}

/**
 * iecm_tx_buf_rel - Release a Tx buffer
 * @tx_q: the queue that owns the buffer
 * @tx_buf: the buffer to free
 */
void iecm_tx_buf_rel(struct iecm_queue *tx_q, struct iecm_tx_buf *tx_buf)
{
	/* stub */
}

/**
 * iecm_tx_buf_rel all - Free any empty Tx buffers
 * @txq: queue to be cleaned
 */
static void iecm_tx_buf_rel_all(struct iecm_queue *txq)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_tx_desc_rel_all - Free Tx Resources for All Queues
 * @vport: virtual port structure
 *
 * Free all transmit software resources
 */
static void iecm_tx_desc_rel_all(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_tx_buf_alloc_all - Allocate memory for all buffer resources
 * @tx_q: queue for which the buffers are allocated
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_tx_buf_alloc_all(struct iecm_queue *tx_q)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_tx_desc_alloc_all - allocate all queues Tx resources
 * @vport: virtual port private structure
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_tx_desc_alloc_all(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_rx_buf_rel - Release a Rx buffer
 * @rxq: the queue that owns the buffer
 * @rx_buf: the buffer to free
 */
static void iecm_rx_buf_rel(struct iecm_queue *rxq,
			    struct iecm_rx_buf *rx_buf)
{
	/* stub */
}

/**
 * iecm_rx_buf_rel_all - Free all Rx buffer resources for a queue
 * @rxq: queue to be cleaned
 */
static void iecm_rx_buf_rel_all(struct iecm_queue *rxq)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_rx_desc_rel_all - Free Rx Resources for All Queues
 * @vport: virtual port structure
 *
 * Free all Rx queues resources
 */
static void iecm_rx_desc_rel_all(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_rx_buf_hw_update - Store the new tail and head values
 * @rxq: queue to bump
 * @val: new head index
 */
void iecm_rx_buf_hw_update(struct iecm_queue *rxq, u32 val)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_rx_buf_alloc_all - Allocate memory for all buffer resources
 * @rxq: queue for which the buffers are allocated
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_rx_buf_alloc_all(struct iecm_queue *rxq)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_rx_desc_alloc_all - allocate all RX queues resources
 * @vport: virtual port structure
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_rx_desc_alloc_all(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_txq_group_rel - Release all resources for txq groups
 * @vport: vport to release txq groups on
 */
static void iecm_txq_group_rel(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_rxq_group_rel - Release all resources for rxq groups
 * @vport: vport to release rxq groups on
 */
static void iecm_rxq_group_rel(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_queue_grp_rel_all - Release all queue groups
 * @vport: vport to release queue groups for
 */
static void iecm_vport_queue_grp_rel_all(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_queues_rel - Free memory for all queues
 * @vport: virtual port
 *
 * Free the memory allocated for queues associated to a vport
 */
void iecm_vport_queues_rel(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_init_num_qs - Initialize number of queues
 * @vport: vport to initialize qs
 * @vport_msg: data to be filled into vport
 */
void iecm_vport_init_num_qs(struct iecm_vport *vport,
			    struct virtchnl_create_vport *vport_msg)
{
	/* stub */
}

/**
 * iecm_vport_calc_num_q_desc - Calculate number of queue groups
 * @vport: vport to calculate q groups for
 */
void iecm_vport_calc_num_q_desc(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_calc_num_q_groups - Calculate number of queue groups
 * @vport: vport to calculate q groups for
 */
void iecm_vport_calc_num_q_groups(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_calc_num_q_vec - Calculate total number of vectors required for
 * this vport
 * @vport: virtual port
 *
 */
void iecm_vport_calc_num_q_vec(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_queue_grp_alloc_all - Allocate all queue groups/resources
 * @vport: vport with qgrps to allocate
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_vport_queue_grp_alloc_all(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_tx_handle_sw_marker - Handle queue marker packet
 * @tx_q: Tx queue to handle software marker
 */
static void iecm_tx_handle_sw_marker(struct iecm_queue *tx_q)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_tx_buf_hw_update - Store the new tail and head values
 * @tx_q: queue to bump
 * @val: new head index
 */
void iecm_tx_buf_hw_update(struct iecm_queue *tx_q, u32 val)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_tx_desc_count_required - calculate number of Tx descriptors needed
 * @skb: send buffer
 *
 * Returns number of data descriptors needed for this skb.
 */
unsigned int iecm_tx_desc_count_required(struct sk_buff *skb)
{
	/* stub */
}

/**
 * iecm_tx_splitq_map - Build the Tx flex descriptor
 * @tx_q: queue to send buffer on
 * @off: pointer to offload params struct
 * @first: first buffer info buffer to use
 *
 * This function loops over the skb data pointed to by *first
 * and gets a physical address for each memory location and programs
 * it and the length into the transmit flex descriptor.
 */
static void
iecm_tx_splitq_map(struct iecm_queue *tx_q,
		   struct iecm_tx_offload_params *off,
		   struct iecm_tx_buf *first)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_rx_hwtstamp - check for an RX timestamp and pass up
 * the stack
 * @rx_desc: pointer to Rx descriptor containing timestamp
 * @skb: skb to put timestamp in
 */
static void iecm_rx_hwtstamp(struct iecm_flex_rx_desc *rx_desc,
			     struct sk_buff __maybe_unused *skb)
{
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_rx_page_is_reserved - check if reuse is possible
 * @page: page struct to check
 */
static bool iecm_rx_page_is_reserved(struct page *page)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_rx_bump_ntc - Bump and wrap q->next_to_clean value
 * @q: queue to bump
 */
static void iecm_rx_bump_ntc(struct iecm_queue *q)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_splitq_napi_poll - NAPI handler
 * @napi: struct from which you get q_vector
 * @budget: budget provided by stack
 */
static int iecm_vport_splitq_napi_poll(struct napi_struct *napi, int budget)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}

/**
 * iecm_init_rss - Prepare for RSS
 * @vport: virtual port
 *
 * Return 0 on success, negative on failure
 */
int iecm_init_rss(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_deinit_rss - Prepare for RSS
 * @vport: virtual port
 */
void iecm_deinit_rss(struct iecm_vport *vport)
{
	/* stub */
}
