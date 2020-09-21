// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020, Intel Corporation. */

#include <linux/net/intel/iecm_controlq.h>

/**
 * iecm_ctlq_alloc_desc_ring - Allocate Control Queue (CQ) rings
 * @hw: pointer to hw struct
 * @cq: pointer to the specific Control queue
 */
static int
iecm_ctlq_alloc_desc_ring(struct iecm_hw *hw,
			  struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_alloc_bufs - Allocate Control Queue (CQ) buffers
 * @hw: pointer to hw struct
 * @cq: pointer to the specific Control queue
 *
 * Allocate the buffer head for all control queues, and if it's a receive
 * queue, allocate DMA buffers
 */
static int iecm_ctlq_alloc_bufs(struct iecm_hw *hw,
				struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_free_desc_ring - Free Control Queue (CQ) rings
 * @hw: pointer to hw struct
 * @cq: pointer to the specific Control queue
 *
 * This assumes the posted send buffers have already been cleaned
 * and de-allocated
 */
static void iecm_ctlq_free_desc_ring(struct iecm_hw *hw,
				     struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_free_bufs - Free CQ buffer info elements
 * @hw: pointer to hw struct
 * @cq: pointer to the specific Control queue
 *
 * Free the DMA buffers for RX queues, and DMA buffer header for both RX and TX
 * queues.  The upper layers are expected to manage freeing of TX DMA buffers
 */
static void iecm_ctlq_free_bufs(struct iecm_hw *hw, struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_dealloc_ring_res - Free memory allocated for control queue
 * @hw: pointer to hw struct
 * @cq: pointer to the specific Control queue
 *
 * Free the memory used by the ring, buffers and other related structures
 */
void iecm_ctlq_dealloc_ring_res(struct iecm_hw *hw, struct iecm_ctlq_info *cq)
{
	/* stub */
}

/**
 * iecm_ctlq_alloc_ring_res - allocate memory for descriptor ring and bufs
 * @hw: pointer to hw struct
 * @cq: pointer to control queue struct
 *
 * Do *NOT* hold the lock when calling this as the memory allocation routines
 * called are not going to be atomic context safe
 */
int iecm_ctlq_alloc_ring_res(struct iecm_hw *hw, struct iecm_ctlq_info *cq)
{
	/* stub */
}
