/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2016-2019 Intel Corporation. */
#ifndef _GSWIP_CORE_H_
#define _GSWIP_CORE_H_

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/regmap.h>

#include "gswip.h"

/* table should be ready in 30 clock cycle */
#define TBL_BUSY_TIMEOUT_US	1

struct core_ops {
	const struct core_common_ops *common_ops;
	const struct core_pmac_ops *pmac_ops;
	const struct core_gpid_ops *gpid_ops;
	const struct core_ctp_ops *ctp_ops;
	const struct core_br_port_ops *br_port_ops;
	const struct core_br_ops *br_ops;
	const struct core_qos_ops *qos_ops;
};

struct gswip_core_priv {
	struct device *dev;

	unsigned long *br_map;
	unsigned long *br_port_map;
	unsigned long *ctp_port_map;
	void *pdev;

	u8 cpu_port;
	u8 num_lport;
	u8 num_br;
	u8 num_br_port;
	u16 num_ctp;
	u16 num_glb_port;
	u16 num_pmac;
	u16 num_phy_port;
	u16 num_q;

	u16 ver;
	struct regmap *regmap;
	/* table read/write lock */
	spinlock_t tbl_lock;

	struct core_ops ops;
};

static inline void update_val(u16 *val, u16 mask, u16 set)
{
	*val &= ~mask;
	*val |= FIELD_PREP(mask, set);
}

static inline void reg_r16(struct gswip_core_priv *priv, u16 reg, u16 *val)
{
	unsigned int reg_val;

	regmap_read(priv->regmap, reg, &reg_val);
	*val = reg_val;
}

static inline void reg_wbits(struct gswip_core_priv *priv,
			     u16 reg, u16 mask, u16 val)
{
	regmap_update_bits(priv->regmap, reg, mask, FIELD_PREP(mask, val));
}

static inline u16 reg_rbits(struct gswip_core_priv *priv, u16 reg, u16 mask)
{
	unsigned int reg_val;

	regmap_read(priv->regmap, reg, &reg_val);
	return FIELD_GET(mask, reg_val);
}

/* Access table with timeout */
static inline int tbl_rw_tmout(struct gswip_core_priv *priv, u16 reg, u16 mask)
{
	unsigned int val;

	return regmap_read_poll_timeout(priv->regmap, reg, val, !(val & mask),
					0, TBL_BUSY_TIMEOUT_US);
}

int gswip_core_setup_port_ops(struct gswip_core_priv *priv);

#endif
