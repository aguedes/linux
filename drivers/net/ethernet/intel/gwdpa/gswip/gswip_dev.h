/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2016-2019 Intel Corporation. */
#ifndef _GSWIP_DEV_H
#define _GSWIP_DEV_H

struct gswip_pdata {
	void __iomem *sw;
	void __iomem *lmac;
	void __iomem *core;
	int sw_irq;
	int core_irq;
	struct clk *ptp_clk;
	struct clk *sw_clk;
	bool intr_flag;
	struct regmap *core_regmap;
};

#endif

