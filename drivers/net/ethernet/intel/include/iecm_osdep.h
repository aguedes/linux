/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020, Intel Corporation. */

#ifndef _IECM_OSDEP_H_
#define _IECM_OSDEP_H_

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include "iecm_alloc.h"

#define wr32(a, reg, value)     writel((value), (u8 *)((a)->hw_addr + (reg)))
#define rd32(a, reg)            readl((u8 *)(a)->hw_addr + (reg))
#define wr64(a, reg, value)     writeq((value), (u8 *)((a)->hw_addr + (reg)))
#define rd64(a, reg)            readq((u8 *)(a)->hw_addr + (reg))

struct iecm_dma_mem {
	void *va;
	dma_addr_t pa;
	size_t size;
};

#define iecm_wmb() wmb() /* memory barrier */
#endif /* _IECM_OSDEP_H_ */
