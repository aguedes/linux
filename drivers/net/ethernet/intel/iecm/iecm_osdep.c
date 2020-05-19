// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020 Intel Corporation. */

#include "iecm_osdep.h"
#include "iecm.h"

void *iecm_alloc_dma_mem(struct iecm_hw *hw, struct iecm_dma_mem *mem, u64 size)
{
	size_t sz = ALIGN(size, 4096);
	struct iecm_adapter *pf = hw->back;

	mem->va = dma_alloc_coherent(&pf->pdev->dev, sz,
				     &mem->pa, GFP_KERNEL | __GFP_ZERO);
	mem->size = size;

	return mem->va;
}

void iecm_free_dma_mem(struct iecm_hw *hw, struct iecm_dma_mem *mem)
{
	struct iecm_adapter *pf = hw->back;

	dma_free_coherent(&pf->pdev->dev, mem->size,
			  mem->va, mem->pa);
	mem->size = 0;
	mem->va = NULL;
	mem->pa = 0;
}
