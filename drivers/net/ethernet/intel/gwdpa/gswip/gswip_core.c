// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation. */

#include <asm/unaligned.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>

#include "gswip_core.h"
#include "gswip_dev.h"
#include "gswip_reg.h"
#include "gswip_tbl.h"
#include "mac_common.h"

#define GSWIP_VER		0x32

/* TX DMA channels for PMAC0 to PMAC2 */
#define PMAC0_TX_DMACHID_START	0
#define PMAC0_TX_DMACHID_END	16
#define PMAC1_TX_DMACHID_START	0
#define PMAC1_TX_DMACHID_END	16
#define PMAC2_TX_DMACHID_START	0
#define PMAC2_TX_DMACHID_END	16

#define MAX_JUMBO_FRM_LEN	10000

/* Data Protocol Unit (DPU) */
enum dpu {
	DPU,
	NON_DPU,
};

enum queue_id {
	QUEUE0,
	QUEUE1,
	QUEUE2,
	QUEUE3,
	QUEUE4,
	QUEUE5,
	QUEUE6,
	QUEUE7,
	QUEUE8,
	QUEUE9,
	QUEUE10,
};

enum traffic_class {
	TC0,
	TC1,
	TC2,
	TC3,
	TC_MAX,
};

struct eg_pce_bypass_path {
	/* egress logical port id */
	u8 eg_pid;
	/* local extracted */
	bool ext;
	/* queue id */
	u8 qid;
	/* redirect logical port id */
	u8 redir_pid;
};

struct ig_pce_bypass_path {
	/* ingress logical port id */
	u8 ig_pid;
	/* local extracted */
	bool ext;
	/* traffic class */
	u8 tc_start;
	u8 tc_end;
	/* queue id */
	u8 qid;
	/* redirect logical port id */
	u8 redir_pid;
};

/* default GSWIP egress PCE bypass path Q-map */
static struct eg_pce_bypass_path eg_pce_byp_path[] = {
	{MAC2,  false, QUEUE0, MAC2 },
	{MAC3,  false, QUEUE1, MAC3 },
	{MAC4,  false, QUEUE2, MAC4 },
	{MAC5,  false, QUEUE3, MAC5 },
	{MAC6,  false, QUEUE4, MAC6 },
	{MAC7,  false, QUEUE5, MAC7 },
	{MAC8,  false, QUEUE6, MAC8 },
	{MAC9,  false, QUEUE7, MAC9 },
	{MAC10, false, QUEUE8, MAC10},
};

/* default GSWIP ingress PCE bypass path Q-map */
static struct ig_pce_bypass_path ig_pce_byp_path[] = {
	{PMAC0, false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC2,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC3,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC4,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC5,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC6,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC7,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC8,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC9,  false, TC0, TC_MAX, QUEUE10, PMAC2},
	{MAC10, false, TC0, TC_MAX, QUEUE10, PMAC2},
};

static inline int pmac_ig_cfg(struct gswip_core_priv *priv, u8 pmac_id, u8 dpu)
{
	const struct core_pmac_ops *pmac_ops =  priv->ops.pmac_ops;
	u8 start[] = { PMAC0_TX_DMACHID_START,
		       PMAC1_TX_DMACHID_START,
		       PMAC2_TX_DMACHID_START };
	u8 end[] = { PMAC0_TX_DMACHID_END,
		     PMAC1_TX_DMACHID_END,
		     PMAC2_TX_DMACHID_END };
	u8 pmac[] = { PMAC0, PMAC1, PMAC2 };
	struct gswip_pmac_ig_cfg ig_cfg = {0};
	int i, ret;

	ig_cfg.pmac_id = pmac_id;
	ig_cfg.err_pkt_disc = true;
	ig_cfg.class_en = true;

	for (i = start[pmac_id]; i < end[pmac_id]; i++) {
		ig_cfg.tx_dma_chan_id = i;
		ig_cfg.def_pmac_hdr[2] = FIELD_PREP(PMAC_IGCFG_HDR_ID,
						    pmac[pmac_id]);

		ret = pmac_ops->ig_cfg_set(priv->dev, &ig_cfg);
		if (ret)
			return ret;
	}

	return 0;
}

static inline int pmac_eg_cfg(struct gswip_core_priv *priv, u8 pmac_id, u8 dpu)
{
	const struct core_pmac_ops *pmac_ops =  priv->ops.pmac_ops;
	struct gswip_pmac_eg_cfg pmac_eg = {0};

	pmac_eg.pmac_id = pmac_id;
	pmac_eg.pmac_en = true;
	pmac_eg.bsl_seg_disable = true;

	return pmac_ops->eg_cfg_set(priv->dev, &pmac_eg);
}

static inline int pmac_glbl_cfg(struct gswip_core_priv *priv, u8 pmac_id)
{
	const struct core_pmac_ops *pmac_ops =  priv->ops.pmac_ops;
	struct gswip_pmac_glb_cfg pmac_cfg = {0};

	pmac_cfg.pmac_id = pmac_id;
	pmac_cfg.rx_fcs_dis = true;
	pmac_cfg.jumbo_en = true;
	pmac_cfg.max_jumbo_len = MAX_JUMBO_FRM_LEN;
	pmac_cfg.long_frm_chk_dis = true;
	pmac_cfg.short_frm_chk_type = GSWIP_PMAC_SHORT_LEN_ENA_UNTAG;
	pmac_cfg.proc_flags_eg_cfg_en = true;
	pmac_cfg.proc_flags_eg_cfg = GSWIP_PMAC_PROC_FLAGS_MIX;

	return pmac_ops->gbl_cfg_set(priv->dev, &pmac_cfg);
}

static int gswip_core_pmac_init_nondpu(struct gswip_core_priv *priv)
{
	int i, ret;

	for (i = 0; i < priv->num_pmac; i++) {
		ret = pmac_glbl_cfg(priv, i);
		if (ret)
			return ret;

		ret = pmac_ig_cfg(priv, i, NON_DPU);
		if (ret)
			return ret;

		ret = pmac_eg_cfg(priv, i, NON_DPU);
		if (ret)
			return ret;
	}

	return 0;
}

static int gswip_core_set_def_eg_pce_bypass_qmap(struct gswip_core_priv *priv,
						 enum gswip_qos_q_map_mode mode)
{
	const struct core_qos_ops *qos_ops =  priv->ops.qos_ops;
	int num_elem = ARRAY_SIZE(eg_pce_byp_path);
	struct gswip_qos_q_port q_map = {0};
	int i, ret;

	q_map.egress = true;
	q_map.q_map_mode = mode;

	for (i = 0; i < num_elem; i++) {
		q_map.lpid = eg_pce_byp_path[i].eg_pid;
		q_map.extration_en = eg_pce_byp_path[i].ext;
		q_map.qid = eg_pce_byp_path[i].qid;
		q_map.redir_port_id = eg_pce_byp_path[i].redir_pid;

		ret = qos_ops->q_port_set(priv->dev, &q_map);
		if (ret)
			return -EINVAL;
	}

	return 0;
}

static int gswip_core_set_def_ig_pce_bypass_qmap(struct gswip_core_priv *priv)
{
	const struct core_qos_ops *qos_ops =  priv->ops.qos_ops;
	int num_elem = ARRAY_SIZE(ig_pce_byp_path);
	struct gswip_qos_q_port q_map = {0};
	int i, j, ret;

	q_map.en_ig_pce_bypass = true;

	for (i = 0; i < num_elem; i++) {
		for (j = ig_pce_byp_path[i].tc_start;
		     j < ig_pce_byp_path[i].tc_end; j++) {
			q_map.lpid = ig_pce_byp_path[i].ig_pid;
			q_map.qid = ig_pce_byp_path[i].qid;
			q_map.redir_port_id = ig_pce_byp_path[i].redir_pid;
			q_map.tc_id = j;

			ret = qos_ops->q_port_set(priv->dev, &q_map);
			if (ret)
				return -EINVAL;
		}
	}

	return 0;
}

static inline u16 pmac_reg(u16 offset, u8 id)
{
	u16 pmac_offset[] = { 0, PMAC_REG_OFFSET_1, PMAC_REG_OFFSET_2 };

	return offset += pmac_offset[id];
}

/* PMAC global configuration */
static int gswip_pmac_glb_cfg_set(struct device *dev,
				  struct gswip_pmac_glb_cfg *pmac)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 bsl_reg[] = { PMAC_BSL_LEN0, PMAC_BSL_LEN1, PMAC_BSL_LEN2 };
	u16 ctrl_reg[] = { PMAC_CTRL_0, PMAC_CTRL_1,
			   PMAC_CTRL_2, PMAC_CTRL_4 };
	u16 ctrl[PMAC_CTRL_NUM] = {0};
	u8 id;
	int i;

	if (pmac->pmac_id >= priv->num_pmac) {
		dev_err(priv->dev, "Invalid pmac id %d\n", pmac->pmac_id);
		return -EINVAL;
	}

	ctrl[0] = FIELD_PREP(PMAC_CTRL_0_FCSEN, pmac->rx_fcs_dis) |
		  FIELD_PREP(PMAC_CTRL_0_APADEN, pmac->apad_en) |
		  FIELD_PREP(PMAC_CTRL_0_VPAD2EN, pmac->svpad_en) |
		  FIELD_PREP(PMAC_CTRL_0_VPADEN, pmac->vpad_en) |
		  FIELD_PREP(PMAC_CTRL_0_PADEN, pmac->pad_en) |
		  FIELD_PREP(PMAC_CTRL_0_FCS, pmac->tx_fcs_dis) |
		  FIELD_PREP(PMAC_CTRL_0_CHKREG, pmac->ip_trans_chk_reg_dis) |
		  FIELD_PREP(PMAC_CTRL_0_CHKVER, pmac->ip_trans_chk_ver_dis);

	if (pmac->jumbo_en) {
		ctrl[1] = pmac->max_jumbo_len,
		ctrl[2] = FIELD_PREP(PMAC_CTRL_2_MLEN, 1);
	}

	ctrl[2] |= FIELD_PREP(PMAC_CTRL_2_LCHKL, pmac->long_frm_chk_dis);

	switch (pmac->short_frm_chk_type) {
	case GSWIP_PMAC_SHORT_LEN_DIS:
		ctrl[2] |= FIELD_PREP(PMAC_CTRL_2_LCHKS, 0);
		break;

	case GSWIP_PMAC_SHORT_LEN_ENA_UNTAG:
		ctrl[2] |= FIELD_PREP(PMAC_CTRL_2_LCHKS, 1);
		break;

	case GSWIP_PMAC_SHORT_LEN_ENA_TAG:
		ctrl[2] |= FIELD_PREP(PMAC_CTRL_2_LCHKS, 2);
		break;

	case GSWIP_PMAC_SHORT_LEN_RESERVED:
		ctrl[2] |= FIELD_PREP(PMAC_CTRL_2_LCHKS, 3);
		break;
	}

	switch (pmac->proc_flags_eg_cfg) {
	case GSWIP_PMAC_PROC_FLAGS_NONE:
		break;

	case GSWIP_PMAC_PROC_FLAGS_TC:
		ctrl[3] |= FIELD_PREP(PMAC_CTRL_4_FLAGEN, 0);
		break;

	case GSWIP_PMAC_PROC_FLAGS_FLAG:
		ctrl[3] |= FIELD_PREP(PMAC_CTRL_4_FLAGEN, 1);
		break;

	case GSWIP_PMAC_PROC_FLAGS_MIX:
		ctrl[3] |= FIELD_PREP(PMAC_CTRL_4_FLAGEN, 2);
		break;
	}

	id = pmac->pmac_id;

	for (i = 0; i < PMAC_CTRL_NUM; i++)
		regmap_write(priv->regmap, pmac_reg(ctrl_reg[i], id), ctrl[i]);

	for (i = 0; i < PMAC_BSL_NUM; i++)
		regmap_write(priv->regmap, pmac_reg(bsl_reg[i], id),
			     pmac->bsl_thresh[i]);

	return 0;
}

/* PMAC backpressure configuration */
static int gswip_pmac_bp_map_get(struct device *dev,
				 struct gswip_pmac_bp_map *bp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct pmac_tbl_prog pmac_tbl = {0};
	int ret;

	pmac_tbl.id = PMAC_BP_MAP;
	pmac_tbl.pmac_id = bp->pmac_id;
	pmac_tbl.addr = FIELD_GET(PMAC_BPMAP_TX_DMA_CH, bp->tx_dma_chan_id);
	pmac_tbl.num_val = PMAC_BP_MAP_TBL_VAL_NUM;

	ret = gswip_pmac_table_read(priv, &pmac_tbl);
	if (ret)
		return -EINVAL;

	bp->rx_port_mask = pmac_tbl.val[0];
	bp->tx_q_mask = pmac_tbl.val[1];
	bp->tx_q_mask |= FIELD_PREP(PMAC_BPMAP_TX_Q_UPPER, pmac_tbl.val[2]);

	return 0;
}

/* PMAC ingress configuration */
static int gswip_pmac_ig_cfg_set(struct device *dev,
				 struct gswip_pmac_ig_cfg *ig)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct pmac_tbl_prog pmac_tbl = {0};
	u16 *val;
	u16 i;

	pmac_tbl.id = PMAC_IG_CFG;
	pmac_tbl.pmac_id = ig->pmac_id;
	pmac_tbl.addr = FIELD_GET(PMAC_IGCFG_TX_DMA_CH, ig->tx_dma_chan_id);

	val = &pmac_tbl.val[0];
	for (i = 0; i < 4; i++)
		val[i] = get_unaligned_be16(&ig->def_pmac_hdr[i * 2]);

	switch (ig->sub_id) {
	case GSWIP_PMAC_IG_CFG_SRC_DMA_DESC:
		break;

	case GSWIP_PMAC_IG_CFG_SRC_PMAC:
	case GSWIP_PMAC_IG_CFG_SRC_DEF_PMAC:
		val[4] = PMAC_IGCFG_VAL4_SUBID_MODE;
		break;
	}

	val[4] |= FIELD_PREP(PMAC_IGCFG_VAL4_PMAC_FLAG, ig->pmac_present) |
		  FIELD_PREP(PMAC_IGCFG_VAL4_CLASSEN_MODE, ig->class_en) |
		  FIELD_PREP(PMAC_IGCFG_VAL4_CLASS_MODE, ig->class_def) |
		  FIELD_PREP(PMAC_IGCFG_VAL4_ERR_DP, ig->err_pkt_disc) |
		  FIELD_PREP(PMAC_IGCFG_VAL4_SPPID_MODE, ig->src_port_id_def);

	pmac_tbl.num_val = PMAC_IG_CFG_TBL_VAL_NUM;

	return gswip_pmac_table_write(priv, &pmac_tbl);
}

/* PMAC egress configuration */
static int gswip_pmac_eg_cfg_set(struct device *dev,
				 struct gswip_pmac_eg_cfg *eg)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct pmac_tbl_prog pmac_tbl = {0};
	u16 ctrl, pmac_ctrl4;
	u16 *val;

	pmac_ctrl4 = pmac_reg(PMAC_CTRL_4, eg->pmac_id);

	pmac_tbl.id = PMAC_EG_CFG;
	pmac_tbl.pmac_id = eg->pmac_id;
	pmac_tbl.addr = FIELD_PREP(PMAC_EGCFG_DST_PORT_ID, eg->dst_port_id)
			| FIELD_PREP(PMAC_EGCFG_FLOW_ID_MSB,
				     eg->flow_id_msb);

	ctrl = reg_rbits(priv, pmac_ctrl4, PMAC_CTRL_4_FLAGEN);

	if (ctrl == PMAC_RX_FSM_IDLE) {
		pmac_tbl.addr |= FIELD_PREP(PMAC_EGCFG_TC_4BITS, eg->tc);
	} else if (ctrl == PMAC_RX_FSM_IGCFG) {
		pmac_tbl.addr |= FIELD_PREP(PMAC_EGCFG_MPE1, eg->mpe1) |
				 FIELD_PREP(PMAC_EGCFG_MPE2, eg->mpe2) |
				 FIELD_PREP(PMAC_EGCFG_ECRYPT, eg->encrypt) |
				 FIELD_PREP(PMAC_EGCFG_DECRYPT, eg->decrypt);
	} else if (ctrl == PMAC_RX_FSM_DASA) {
		pmac_tbl.addr |= FIELD_PREP(PMAC_EGCFG_TC_2BITS, eg->tc) |
				 FIELD_PREP(PMAC_EGCFG_MPE1, eg->mpe1) |
				 FIELD_PREP(PMAC_EGCFG_MPE2, eg->mpe2);
	}

	val = &pmac_tbl.val[0];
	val[0] = FIELD_PREP(PMAC_EGCFG_VAL0_REDIR, eg->redir_en) |
		 FIELD_PREP(PMAC_EGCFG_VAL0_RES_3BITS, ctrl) |
		   FIELD_PREP(PMAC_EGCFG_VAL0_BSL, eg->bsl_tc) |
		   FIELD_PREP(PMAC_EGCFG_VAL0_RES_2BITS, eg->resv_2dw0);

	val[1] = FIELD_PREP(PMAC_EGCFG_VAL1_RX_DMA_CH, eg->rx_dma_chan_id);

	val[2] = FIELD_PREP(PMAC_EGCFG_VAL2_FCS_MODE, eg->fcs_en) |
		   FIELD_PREP(PMAC_EGCFG_VAL2_PMAC_FLAG, eg->pmac_en);

	if (eg->rm_l2_hdr) {
		val[2] |= PMAC_EGCFG_VAL2_L2HD_RM_MODE |
			    FIELD_PREP(PMAC_EGCFG_VAL2_L2HD_RM,
				       eg->num_byte_rm);
	}

	pmac_tbl.num_val = PMAC_EG_CFG_TBL_VAL_NUM;

	return gswip_pmac_table_write(priv, &pmac_tbl);
}

static int gswip_cpu_port_cfg_get(struct device *dev,
				  struct gswip_cpu_port_cfg *cpu)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 lpid, val;

	lpid = cpu->lpid;
	if (lpid >= priv->num_lport) {
		dev_err(priv->dev, "Invalid cpu port id %d > %d\n",
			lpid, priv->num_lport);
		return -EINVAL;
	}

	cpu->is_cpu_port = lpid == priv->cpu_port;
	cpu->spec_tag_ig = reg_rbits(priv, PCE_PCTRL_0(lpid),
				     PCE_PCTRL_0_IGSTEN);
	cpu->fcs_chk = reg_rbits(priv, SDMA_PCTRL(lpid), SDMA_PCTRL_FCSIGN);

	reg_r16(priv, FDMA_PASR, &val);
	cpu->no_mpe_parser_cfg = FIELD_GET(FDMA_PASR_CPU, val);
	cpu->mpe1_parser_cfg = FIELD_GET(FDMA_PASR_MPE1, val);
	cpu->mpe2_parser_cfg = FIELD_GET(FDMA_PASR_MPE2, val);
	cpu->mpe3_parser_cfg = FIELD_GET(FDMA_PASR_MPE3, val);

	reg_r16(priv, FDMA_PCTRL(lpid), &val);
	cpu->spec_tag_eg = FIELD_GET(FDMA_PCTRL_STEN, val);
	cpu->ts_rm_ptp_pkt = FIELD_GET(FDMA_PCTRL_TS_PTP, val);
	cpu->ts_rm_non_ptp_pkt = FIELD_GET(FDMA_PCTRL_TS_NONPTP, val);

	return 0;
}

static int gswip_register_get(struct device *dev, struct gswip_register *param)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);

	reg_r16(priv, param->offset, &param->data);

	return 0;
}

static int gswip_register_set(struct device *dev, struct gswip_register *param)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);

	regmap_write(priv->regmap, param->offset, param->data);

	return 0;
}

/* Global Port ID/ Logical Port ID */
static int gswip_lpid2gpid_set(struct device *dev,
			       struct gswip_lpid2gpid *lp2gp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 lpid, first_gpid, last_gpid, val;

	lpid = lp2gp->lpid;
	first_gpid = lp2gp->first_gpid;
	last_gpid = first_gpid + lp2gp->num_gpid - 1;

	if (lpid >= priv->num_lport) {
		dev_err(priv->dev, "Invalid lpid %d >= %d\n",
			lpid, priv->num_lport);
		return -EINVAL;
	}

	if (last_gpid >= priv->num_glb_port) {
		dev_err(priv->dev, "Invalid last gpid %d >= %d\n",
			last_gpid, priv->num_glb_port);
		return -EINVAL;
	}

	val = FIELD_PREP(ETHSW_GPID_STARTID_STARTID, first_gpid) |
	      FIELD_PREP(ETHSW_GPID_STARTID_BITS, lp2gp->valid_bits);
	regmap_write(priv->regmap, ETHSW_GPID_STARTID(lpid), val);

	reg_wbits(priv, ETHSW_GPID_ENDID(lpid),
		  ETHSW_GPID_ENDID_ENDID, last_gpid);

	return 0;
}

static int gswip_lpid2gpid_get(struct device *dev,
			       struct gswip_lpid2gpid *lp2gp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 lpid, val;

	lpid = lp2gp->lpid;
	if (lpid >= priv->num_lport) {
		dev_err(priv->dev, "Invalid lpid %d >= %d\n",
			lpid, priv->num_lport);
		return -EINVAL;
	}

	reg_r16(priv, ETHSW_GPID_STARTID(lpid), &val);
	lp2gp->first_gpid = FIELD_GET(ETHSW_GPID_STARTID_STARTID, val);
	lp2gp->valid_bits = FIELD_GET(ETHSW_GPID_STARTID_BITS, val);

	reg_r16(priv, ETHSW_GPID_ENDID(lpid), &val);
	lp2gp->num_gpid = val - lp2gp->first_gpid + 1;

	return 0;
}

static int gswip_gpid2lpid_set(struct device *dev,
			       struct gswip_gpid2lpid *gp2lp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 val = 0;

	if (gp2lp->lpid >= priv->num_lport) {
		dev_err(priv->dev, "Invalid lpid %d >= %d\n",
			gp2lp->lpid, priv->num_lport);
		return -EINVAL;
	}

	if (gp2lp->gpid >= priv->num_glb_port) {
		dev_err(priv->dev, "Invalid gpid %d >= %d\n",
			gp2lp->gpid, priv->num_glb_port);
		return -EINVAL;
	}

	val = FIELD_PREP(GPID_RAM_VAL_LPID, gp2lp->lpid) |
	      FIELD_PREP(GPID_RAM_VAL_SUBID_GRP, gp2lp->subif_grp_field) |
	      FIELD_PREP(GPID_RAM_VAL_OV, gp2lp->subif_grp_field_ovr);

	regmap_write(priv->regmap, GPID_RAM_VAL, val);

	if (tbl_rw_tmout(priv, GPID_RAM_CTRL, GPID_RAM_CTRL_BAS)) {
		dev_err(priv->dev, "failed to access gpid table\n");
		return -EBUSY;
	}

	reg_r16(priv, GPID_RAM_CTRL, &val);

	update_val(&val, GPID_RAM_CTRL_ADDR, gp2lp->gpid);
	val |= FIELD_PREP(GPID_RAM_CTRL_OPMOD, 1) |
	       FIELD_PREP(GPID_RAM_CTRL_BAS, 1);

	regmap_write(priv->regmap, GPID_RAM_CTRL, val);

	if (tbl_rw_tmout(priv, GPID_RAM_CTRL, GPID_RAM_CTRL_BAS)) {
		dev_err(priv->dev, "failed to write gpid table\n");
		return -EBUSY;
	}

	return 0;
}

static int gswip_gpid2lpid_get(struct device *dev,
			       struct gswip_gpid2lpid *gp2lp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 val;

	if (gp2lp->gpid >= priv->num_glb_port) {
		dev_err(priv->dev, "gpid %d >= %d\n",
			gp2lp->gpid, priv->num_glb_port);
		return -EINVAL;
	}

	if (tbl_rw_tmout(priv, GPID_RAM_CTRL, GPID_RAM_CTRL_BAS)) {
		dev_err(priv->dev, "failed to access gpid table\n");
		return -EBUSY;
	}

	reg_r16(priv, GPID_RAM_CTRL, &val);

	update_val(&val, GPID_RAM_CTRL_ADDR, gp2lp->gpid);
	val |= FIELD_PREP(GPID_RAM_CTRL_OPMOD, 0) |
	       FIELD_PREP(GPID_RAM_CTRL_BAS, 1);

	regmap_write(priv->regmap, GPID_RAM_CTRL, val);

	if (tbl_rw_tmout(priv, GPID_RAM_CTRL, GPID_RAM_CTRL_BAS)) {
		dev_err(priv->dev, "failed to read gpid table\n");
		return -EBUSY;
	}

	reg_r16(priv, GPID_RAM_VAL, &val);

	gp2lp->lpid = FIELD_GET(GPID_RAM_VAL_LPID, val);
	gp2lp->subif_grp_field = FIELD_GET(GPID_RAM_VAL_SUBID_GRP, val);
	gp2lp->subif_grp_field_ovr = FIELD_GET(GPID_RAM_VAL_OV, val);

	return 0;
}

static int gswip_enable(struct device *dev, bool enable)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 i;

	for (i = 0; i < priv->num_lport; i++) {
		reg_wbits(priv, FDMA_PCTRL(i), FDMA_PCTRL_EN, enable);
		reg_wbits(priv, SDMA_PCTRL(i), SDMA_PCTRL_PEN, enable);
	}

	return 0;
}

static const struct core_common_ops gswip_core_common_ops = {
	.enable = gswip_enable,
	.cpu_port_cfg_get = gswip_cpu_port_cfg_get,
	.reg_get = gswip_register_get,
	.reg_set = gswip_register_set,
};

static const struct core_pmac_ops gswip_core_pmac_ops = {
	.gbl_cfg_set = gswip_pmac_glb_cfg_set,
	.bp_map_get = gswip_pmac_bp_map_get,
	.ig_cfg_set = gswip_pmac_ig_cfg_set,
	.eg_cfg_set = gswip_pmac_eg_cfg_set,
};

static const struct core_gpid_ops gswip_core_gpid_ops = {
	.lpid2gpid_set = gswip_lpid2gpid_set,
	.lpid2gpid_get = gswip_lpid2gpid_get,
	.gpid2lpid_set = gswip_gpid2lpid_set,
	.gpid2lpid_get = gswip_gpid2lpid_get,
};

static int gswip_core_setup_ops(struct gswip_core_priv *priv)
{
	struct core_ops *ops =  &priv->ops;

	ops->common_ops = &gswip_core_common_ops;
	ops->pmac_ops = &gswip_core_pmac_ops;
	ops->gpid_ops = &gswip_core_gpid_ops;

	return 0;
}

static int gswip_core_get_hw_cap(struct gswip_core_priv *priv)
{
	u16 val;

	priv->ver = reg_rbits(priv, ETHSW_VERSION, ETHSW_VERSION_REV_ID);
	if (priv->ver != GSWIP_VER) {
		dev_err(priv->dev, "Wrong hardware ip version %d\n",
			priv->ver);
		return -EINVAL;
	}

	priv->num_phy_port = reg_rbits(priv, ETHSW_CAP_1, ETHSW_CAP_1_PPORTS);

	reg_r16(priv, ETHSW_CAP_1, &val);
	priv->num_lport = priv->num_phy_port;
	priv->num_lport += FIELD_GET(ETHSW_CAP_1_VPORTS, val);
	priv->num_q = FIELD_GET(ETHSW_CAP_1_QUEUE, val);

	priv->num_pmac = reg_rbits(priv, ETHSW_CAP_13, ETHSW_CAP_13_PMAC);

	reg_r16(priv, ETHSW_CAP_17, &val);
	priv->num_br = (1 << FIELD_GET(ETHSW_CAP_17_BRG, val));
	priv->num_br_port = (1 << FIELD_GET(ETHSW_CAP_17_BRGPT, val));

	reg_r16(priv, ETHSW_CAP_18, &priv->num_ctp);

	priv->num_glb_port = priv->num_br_port * 2;

	return 0;
}

static int gswip_core_init(struct gswip_core_priv *priv)
{
	priv->br_map = devm_kzalloc(priv->dev, BITS_TO_LONGS(priv->num_br),
				    GFP_KERNEL);
	if (!priv->br_map)
		return -ENOMEM;

	priv->br_port_map = devm_kzalloc(priv->dev,
					 BITS_TO_LONGS(priv->num_br_port),
					 GFP_KERNEL);
	if (!priv->br_port_map)
		return -ENOMEM;

	priv->ctp_port_map = devm_kzalloc(priv->dev,
					  BITS_TO_LONGS(priv->num_ctp),
					  GFP_KERNEL);
	if (!priv->ctp_port_map)
		return -ENOMEM;

	spin_lock_init(&priv->tbl_lock);

	gswip_core_setup_ops(priv);
	gswip_core_setup_port_ops(priv);

	return 0;
}

static int gswip_core_setup(struct gswip_core_priv *priv)
{
	int ret;

	ret = gswip_core_set_def_eg_pce_bypass_qmap(priv,
						    GSWIP_QOS_QMAP_SINGLE_MD);
	if (ret)
		return ret;

	ret = gswip_core_set_def_ig_pce_bypass_qmap(priv);
	if (ret)
		return ret;

	ret = gswip_core_pmac_init_nondpu(priv);
	if (ret)
		return -EINVAL;

	return 0;
}

static int gswip_core_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device *parent = dev->parent;
	struct gswip_core_priv *priv;
	struct gswip_pdata *pdata = parent->platform_data;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->pdev = pdev;
	priv->dev = dev;
	priv->regmap = pdata->core_regmap;

	platform_set_drvdata(pdev, priv);

	ret = gswip_core_get_hw_cap(priv);
	if (ret)
		return -EINVAL;

	ret = gswip_core_init(priv);
	if (ret)
		return ret;

	ret = gswip_core_setup(priv);
	if (ret)
		return ret;

	return 0;
}

static const struct of_device_id gswip_core_of_match_table[] = {
	{ .compatible = "gswip-core" },
	{}
};

MODULE_DEVICE_TABLE(of, gswip_core);

static struct platform_driver gswip_core_drv = {
	.probe = gswip_core_probe,
	.driver = {
		.name = "gswip_core",
		.of_match_table = gswip_core_of_match_table,
	},
};

module_platform_driver(gswip_core_drv);

MODULE_LICENSE("GPL v2");
