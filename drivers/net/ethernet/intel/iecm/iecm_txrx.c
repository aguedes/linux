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
	/* stub */
}

/**
 * iecm_vport_intr_napi_dis_all - Disable NAPI for all q_vectors in the vport
 * @vport: main vport structure
 */
static void iecm_vport_intr_napi_dis_all(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_intr_rel - Free memory allocated for interrupt vectors
 * @vport: virtual port
 *
 * Free the memory allocated for interrupt vectors  associated to a vport
 */
void iecm_vport_intr_rel(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_intr_rel_irq - Free the IRQ association with the OS
 * @vport: main vport structure
 */
static void iecm_vport_intr_rel_irq(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_intr_dis_irq_all - Disable each interrupt
 * @vport: main vport structure
 */
void iecm_vport_intr_dis_irq_all(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

static unsigned int iecm_itr_divisor(struct iecm_q_vector *q_vector)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_intr_update_itr_ena_irq - Update ITR and re-enable MSIX interrupt
 * @q_vector: q_vector for which ITR is being updated and interrupt enabled
 */
void iecm_vport_intr_update_itr_ena_irq(struct iecm_q_vector *q_vector)
{
	/* stub */
}

/**
 * iecm_vport_intr_req_irq - get MSI-X vectors from the OS for the vport
 * @vport: main vport structure
 * @basename: name for the vector
 */
static int
iecm_vport_intr_req_irq(struct iecm_vport *vport, char *basename)
{
	/* stub */
}

/**
 * iecm_vport_intr_ena_irq_all - Enable IRQ for the given vport
 * @vport: main vport structure
 */
void iecm_vport_intr_ena_irq_all(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_intr_deinit - Release all vector associations for the vport
 * @vport: main vport structure
 */
void iecm_vport_intr_deinit(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_intr_napi_ena_all - Enable NAPI for all q_vectors in the vport
 * @vport: main vport structure
 */
static void
iecm_vport_intr_napi_ena_all(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_intr_init_vec_idx - Initialize the vector indexes
 * @vport: virtual port
 *
 * Initialize vector indexes with values returned over mailbox
 */
static int iecm_vport_intr_init_vec_idx(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_intr_init - Setup all vectors for the given vport
 * @vport: virtual port
 *
 * Returns 0 on success or negative on failure
 */
int iecm_vport_intr_init(struct iecm_vport *vport)
{
	/* stub */
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
