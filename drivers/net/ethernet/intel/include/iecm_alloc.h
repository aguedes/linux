/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020, Intel Corporation. */

#ifndef _IECM_ALLOC_H_
#define _IECM_ALLOC_H_

/* Memory types */
enum iecm_memset_type {
	IECM_NONDMA_MEM = 0,
	IECM_DMA_MEM
};

/* Memcpy types */
enum iecm_memcpy_type {
	IECM_NONDMA_TO_NONDMA = 0,
	IECM_NONDMA_TO_DMA,
	IECM_DMA_TO_DMA,
	IECM_DMA_TO_NONDMA
};

struct iecm_hw;
struct iecm_dma_mem;

/* prototype for functions used for dynamic memory allocation */
void *iecm_alloc_dma_mem(struct iecm_hw *hw, struct iecm_dma_mem *mem,
			 u64 size);
void iecm_free_dma_mem(struct iecm_hw *hw, struct iecm_dma_mem *mem);

#endif /* _IECM_ALLOC_H_ */
