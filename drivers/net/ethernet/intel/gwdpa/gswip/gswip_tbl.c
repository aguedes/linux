// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation.*/
#include <linux/kernel.h>

#include "gswip_core.h"
#include "gswip_reg.h"
#include "gswip_tbl.h"

enum tbl_opmode {
	TBL_RD,
	TBL_WR,
};

enum tbl_busy_indication {
	TBL_READY,
	TBL_BUSY,
};

enum pce_tbl_opmode {
	/* Address-based read access  */
	PCE_OPMODE_ADRD,
	/* Address-based write access */
	PCE_OPMODE_ADWR,
	/* Key-based read access      */
	PCE_OPMODE_KSRD,
	/* Key-based write access     */
	PCE_OPMODE_KSWR
};

struct pce_tbl_config {
	u16 num_key;
	u16 num_mask;
	u16 num_val;
};

struct pce_tbl_reg_map {
	const u16 *key;
	const u16 *mask;
	const u16 *value;
};

static const u16 pce_tbl_key[] = {
	PCE_TBL_KEY_0, PCE_TBL_KEY_1,   PCE_TBL_KEY_2, PCE_TBL_KEY_3,
	PCE_TBL_KEY_4, PCE_TBL_KEY_5,   PCE_TBL_KEY_6, PCE_TBL_KEY_7,
	PCE_TBL_KEY_8, PCE_TBL_KEY_9,   PCE_TBL_KEY_10, PCE_TBL_KEY_11,
	PCE_TBL_KEY_12, PCE_TBL_KEY_13, PCE_TBL_KEY_14, PCE_TBL_KEY_15,
	PCE_TBL_KEY_16, PCE_TBL_KEY_17, PCE_TBL_KEY_18, PCE_TBL_KEY_19,
	PCE_TBL_KEY_20, PCE_TBL_KEY_21, PCE_TBL_KEY_22, PCE_TBL_KEY_23,
	PCE_TBL_KEY_24, PCE_TBL_KEY_25, PCE_TBL_KEY_26, PCE_TBL_KEY_27,
	PCE_TBL_KEY_28, PCE_TBL_KEY_29, PCE_TBL_KEY_30, PCE_TBL_KEY_31,
	PCE_TBL_KEY_32, PCE_TBL_KEY_33,
};

static const u16 pce_tbl_mask[] = {
	PCE_TBL_MASK_0, PCE_TBL_MASK_1, PCE_TBL_MASK_2, PCE_TBL_MASK_3,
};

static const u16 pce_tbl_value[] = {
	PCE_TBL_VAL_0, PCE_TBL_VAL_1,   PCE_TBL_VAL_2, PCE_TBL_VAL_3,
	PCE_TBL_VAL_4, PCE_TBL_VAL_5,   PCE_TBL_VAL_6, PCE_TBL_VAL_7,
	PCE_TBL_VAL_8, PCE_TBL_VAL_9,   PCE_TBL_VAL_10, PCE_TBL_VAL_11,
	PCE_TBL_VAL_12, PCE_TBL_VAL_13, PCE_TBL_VAL_14, PCE_TBL_VAL_15,
	PCE_TBL_VAL_16, PCE_TBL_VAL_17, PCE_TBL_VAL_18, PCE_TBL_VAL_19,
	PCE_TBL_VAL_20, PCE_TBL_VAL_21, PCE_TBL_VAL_22, PCE_TBL_VAL_23,
	PCE_TBL_VAL_24, PCE_TBL_VAL_25, PCE_TBL_VAL_26, PCE_TBL_VAL_27,
	PCE_TBL_VAL_28, PCE_TBL_VAL_29, PCE_TBL_VAL_30,
};

/* PCE table default entries for Key, Mask and Value.
 * Only minimum entries settings are required by default.
 */
static const struct pce_tbl_config pce_tbl_def_cfg[] = {
	/* Parser ucode table                      */
	{ 0,  0,  4 },
	/* Dummy                                   */
	{ 0,  0,  0 },
	/* VLAN filter table                       */
	{ 1,  0,  1 },
	/* PPPoE table                             */
	{ 1,  0,  0 },
	/* Protocol table                          */
	{ 1,  1,  0 },
	/* Application table                       */
	{ 1,  1,  0 },
	/* IP DA/SA MSB table                      */
	{ 4,  4,  0 },
	/* IP DA/SA LSB table                      */
	{ 4,  4,  0 },
	/* Packet length table                     */
	{ 1,  1,  0 },
	/* Inner PCP/DEI table                     */
	{ 0,  0,  1 },
	/* DSCP table                              */
	{ 0,  0,  1 },
	/* MAC bridging table                      */
	{ 4,  0,  10},
	/* DSCP2PCP configuration table            */
	{ 0,  0,  2 },
	/* Multicast SW table                      */
	{ 19, 0,  10},
	/* Dummy                                   */
	{ 0,  0,  0 },
	/* Traffic Flow table                      */
	{ 34, 0,  31},
	/* PBB tunnel template configuration table */
	{ 0,  0,  11},
	/* Queue mapping table                     */
	{ 0,  0,  1 },
	/* Ingress CTP port configuration table    */
	{ 0,  0,  9 },
	/* Egress CTP port configuration table     */
	{ 0,  0,  7 },
	/* Ingress bridge port configuration table */
	{ 0,  0,  18},
	/* Egress bridge port configuration table  */
	{ 0,  0,  14},
	/* MAC DA table                            */
	{ 3,  1,  0 },
	/* MAC SA table                            */
	{ 3,  1,  0 },
	/* Flags table                             */
	{ 1,  1,  0 },
	/* Bridge configuration table              */
	{ 0,  0,  10},
	/* Outer PCP/DEI table                     */
	{ 0,  0,  1 },
	/* Color marking table                     */
	{ 0,  0,  1 },
	/* Color remarking table                   */
	{ 0,  0,  1 },
	/* Payload table                           */
	{ 1,  1,  0 },
	/* Extended VLAN operation table           */
	{ 4,  0,  6 },
	/* P-mapping configuration table           */
	{ 0,  0,  1 },
};

int gswip_pce_table_write(struct gswip_core_priv *priv,
			  struct pce_tbl_prog *pce_tbl)
{
	struct regmap *regmap = priv->regmap;
	u16 i, ctrl;

	spin_lock(&priv->tbl_lock);

	/* update key registers */
	for (i = 0; i < pce_tbl_def_cfg[pce_tbl->id].num_key; i++)
		regmap_write(regmap, pce_tbl_key[i], pce_tbl->key[i]);

	/* update mask registers */
	for (i = 0; i < pce_tbl_def_cfg[pce_tbl->id].num_mask; i++)
		regmap_write(regmap, pce_tbl_mask[i], pce_tbl->mask[i]);

	/* update value registers */
	for (i = 0; i < pce_tbl_def_cfg[pce_tbl->id].num_val; i++)
		regmap_write(regmap, pce_tbl_value[i], pce_tbl->val[i]);

	ctrl = FIELD_PREP(PCE_TBL_CTRL_ADDR, pce_tbl->id) |
	       FIELD_PREP(PCE_TBL_CTRL_OPMOD, PCE_OPMODE_ADWR) |
	       FIELD_PREP(PCE_TBL_CTRL_BAS, TBL_BUSY);

	/* update the pce table */
	regmap_write(regmap, PCE_TBL_ADDR, pce_tbl->addr);
	regmap_write(regmap, PCE_TBL_CTRL, ctrl);

	if (tbl_rw_tmout(priv, PCE_TBL_CTRL, PCE_TBL_CTRL_BAS)) {
		dev_err(priv->dev, "failed to write pce table\n");
		spin_unlock(&priv->tbl_lock);
		return -EBUSY;
	}

	spin_unlock(&priv->tbl_lock);

	return 0;
}

int gswip_pce_table_read(struct gswip_core_priv *priv,
			 struct pce_tbl_prog *pce_tbl)
{
	u16 i, ctrl;

	spin_lock(&priv->tbl_lock);

	regmap_write(priv->regmap, PCE_TBL_ADDR, pce_tbl->addr);

	ctrl = FIELD_PREP(PCE_TBL_CTRL_ADDR, pce_tbl->id) |
	       FIELD_PREP(PCE_TBL_CTRL_OPMOD, PCE_OPMODE_ADRD) |
	       FIELD_PREP(PCE_TBL_CTRL_BAS, TBL_BUSY);
	regmap_write(priv->regmap, PCE_TBL_CTRL, ctrl);

	if (tbl_rw_tmout(priv, PCE_TBL_CTRL, PCE_TBL_CTRL_BAS)) {
		dev_err(priv->dev, "failed to read pce table\n");
		spin_unlock(&priv->tbl_lock);
		return -EBUSY;
	}

	for (i = 0; i < pce_tbl_def_cfg[pce_tbl->id].num_key; i++)
		reg_r16(priv, pce_tbl_key[i], &pce_tbl->key[i]);

	for (i = 0; i < pce_tbl_def_cfg[pce_tbl->id].num_mask; i++)
		reg_r16(priv, pce_tbl_mask[i], &pce_tbl->mask[i]);

	for (i = 0; i < pce_tbl_def_cfg[pce_tbl->id].num_val; i++)
		reg_r16(priv, pce_tbl_value[i], &pce_tbl->val[i]);

	spin_unlock(&priv->tbl_lock);

	return 0;
}

int gswip_bm_table_write(struct gswip_core_priv *priv,
			 struct bm_tbl_prog *bm_tbl)
{
	struct regmap *regmap = priv->regmap;
	u16 ctrl;
	int i;

	spin_lock(&priv->tbl_lock);

	for (i = 0; i < bm_tbl->num_val; i++)
		regmap_write(regmap, (BM_RAM_VAL_0 - i * BM_RAM_VAL_OFFSET),
			     bm_tbl->val[i]);

	regmap_write(regmap, BM_RAM_ADDR, bm_tbl->addr);

	ctrl = bm_tbl->id;
	ctrl |= FIELD_PREP(BM_RAM_CTRL_OPMOD, TBL_WR);
	ctrl |= FIELD_PREP(BM_RAM_CTRL_BAS, TBL_BUSY);
	regmap_write(regmap, BM_RAM_CTRL, ctrl);

	if (tbl_rw_tmout(priv, BM_RAM_CTRL, BM_RAM_CTRL_BAS)) {
		dev_err(priv->dev, "failed to write bm table\n");
		spin_unlock(&priv->tbl_lock);
		return -EBUSY;
	}

	spin_unlock(&priv->tbl_lock);

	return 0;
}

int gswip_bm_table_read(struct gswip_core_priv *priv,
			struct bm_tbl_prog *bm_tbl)
{
	u16 ctrl;
	int i;

	spin_lock(&priv->tbl_lock);

	regmap_write(priv->regmap, BM_RAM_ADDR, bm_tbl->addr);

	ctrl = FIELD_PREP(BM_RAM_CTRL_ADDR, bm_tbl->id) |
	       FIELD_PREP(BM_RAM_CTRL_OPMOD, TBL_RD) |
	       FIELD_PREP(BM_RAM_CTRL_BAS, TBL_BUSY);
	regmap_write(priv->regmap, BM_RAM_CTRL, ctrl);

	if (tbl_rw_tmout(priv, BM_RAM_CTRL, BM_RAM_CTRL_BAS)) {
		dev_err(priv->dev, "failed to read bm table\n");
		spin_unlock(&priv->tbl_lock);
		return -EBUSY;
	}

	for (i = 0; i < bm_tbl->num_val; i++)
		reg_r16(priv, (BM_RAM_VAL_0 - i * BM_RAM_VAL_OFFSET),
			&bm_tbl->val[i]);

	spin_unlock(&priv->tbl_lock);

	return 0;
}

int gswip_pmac_table_write(struct gswip_core_priv *priv,
			   struct pmac_tbl_prog *pmac_tbl)
{
	u16 pmac_id = pmac_tbl->pmac_id, ctrl;
	int i;

	spin_lock(&priv->tbl_lock);

	for (i = 0; i < pmac_tbl->num_val; i++)
		regmap_write(priv->regmap,
			     (PMAC_TBL_VAL(pmac_id) - i * PMAC_TBL_VAL_SFT),
			     pmac_tbl->val[i]);

	regmap_write(priv->regmap, PMAC_TBL_ADDR(pmac_id), pmac_tbl->addr);

	ctrl = FIELD_PREP(PMAC_TBL_CTRL_ADDR, pmac_tbl->id) |
	       FIELD_PREP(PMAC_TBL_CTRL_OPMOD, TBL_WR) |
	       FIELD_PREP(PMAC_TBL_CTRL_BAS, TBL_BUSY);
	regmap_write(priv->regmap, PMAC_TBL_CTRL(pmac_id), ctrl);

	if (tbl_rw_tmout(priv, PMAC_TBL_CTRL(pmac_id), PMAC_TBL_CTRL_BAS)) {
		dev_err(priv->dev, "failed to write pmac table\n");
		spin_unlock(&priv->tbl_lock);
		return -EBUSY;
	}

	spin_unlock(&priv->tbl_lock);

	return 0;
}

int gswip_pmac_table_read(struct gswip_core_priv *priv,
			  struct pmac_tbl_prog *pmac_tbl)
{
	u16 pmac_id = pmac_tbl->pmac_id, ctrl;
	int i;

	spin_lock(&priv->tbl_lock);

	regmap_write(priv->regmap, PMAC_TBL_ADDR(pmac_id), pmac_tbl->addr);

	ctrl = FIELD_PREP(PMAC_TBL_CTRL_ADDR, pmac_tbl->id) |
	       FIELD_PREP(PMAC_TBL_CTRL_OPMOD, TBL_RD) |
	       FIELD_PREP(PMAC_TBL_CTRL_BAS, TBL_BUSY);
	regmap_write(priv->regmap, PMAC_TBL_CTRL(pmac_id), ctrl);

	if (tbl_rw_tmout(priv, PMAC_TBL_CTRL(pmac_id), PMAC_TBL_CTRL_BAS)) {
		dev_err(priv->dev, "failed to read pmac table\n");
		spin_unlock(&priv->tbl_lock);
		return -EBUSY;
	}

	for (i = 0; i < pmac_tbl->num_val; i++)
		reg_r16(priv, (PMAC_TBL_VAL(pmac_id) - i * PMAC_TBL_VAL_SFT),
			&pmac_tbl->val[i]);

	spin_unlock(&priv->tbl_lock);

	return 0;
}
