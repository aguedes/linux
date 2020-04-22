// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation.*/
#include "gswip_core.h"
#include "gswip_reg.h"
#include "gswip_tbl.h"

#define ENABLE		1
#define DISABLE		0

static int gswip_ctp_port_set(struct gswip_core_priv *priv,
			      struct gswip_ctp_port_info *ctp)
{
	u16 val, last_port;

	val = ctp->first_pid;
	last_port = ctp->first_pid + ctp->num_port - 1;

	switch (ctp->mode) {
	case GSWIP_LPORT_8BIT_WLAN:
		val |= FIELD_PREP(ETHSW_CTP_STARTID_MD, MD_WLAN8);
		break;

	case GSWIP_LPORT_9BIT_WLAN:
		val |= FIELD_PREP(ETHSW_CTP_STARTID_MD, MD_WLAN8);
		break;
	case GSWIP_LPORT_GPON:
	case GSWIP_LPORT_EPON:
	case GSWIP_LPORT_GINT:
	case GSWIP_LPORT_OTHER:
		val |= FIELD_PREP(ETHSW_CTP_STARTID_MD, MD_OTHER);
		break;
	}

	regmap_write(priv->regmap, ETHSW_CTP_STARTID(ctp->lpid), val);
	regmap_write(priv->regmap, ETHSW_CTP_ENDID(ctp->lpid), last_port);

	return 0;
}

static int gswip_ctp_port_get(struct gswip_core_priv *priv,
			      struct gswip_ctp_port_info *ctp)
{
	u16 val, last_port;

	reg_r16(priv, ETHSW_CTP_STARTID(ctp->lpid), &val);

	ctp->mode = FIELD_GET(ETHSW_CTP_STARTID_MD, val);
	ctp->first_pid = FIELD_GET(ETHSW_CTP_STARTID_STARTID, val);

	last_port = reg_rbits(priv, ETHSW_CTP_ENDID(ctp->lpid),
			      ETHSW_CTP_ENDID_ENDID);
	ctp->num_port = last_port - ctp->first_pid + 1;

	return 0;
}

static int gswip_ctp_port_alloc(struct device *dev,
				struct gswip_ctp_port_info *ctp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	u16 first_ctp;
	int ret;

	if (!ctp->num_port || ctp->num_port >= priv->num_ctp) {
		dev_err(priv->dev, "Invalid num of ctp port requested %d\n",
			ctp->num_port);
		return -EINVAL;
	}

	first_ctp = bitmap_find_next_zero_area(priv->ctp_port_map,
					       priv->num_ctp, 0,
					       ctp->num_port, 0);
	if (first_ctp >= priv->num_ctp) {
		dev_err(priv->dev, "Failed to find contiguous ctp port\n");
		return -EINVAL;
	}

	bitmap_set(priv->ctp_port_map, first_ctp, ctp->num_port);
	ctp->first_pid = first_ctp;

	ret = gswip_ctp_port_set(priv, ctp);
	if (ret) {
		bitmap_clear(priv->ctp_port_map, first_ctp, ctp->num_port);
		return ret;
	}

	reg_wbits(priv, SDMA_PCTRL(ctp->lpid), SDMA_PCTRL_PEN, ENABLE);

	return 0;
}

static int gswip_ctp_port_free(struct device *dev, u8 lpid)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct gswip_ctp_port_info ctp;

	if (lpid >= priv->num_lport) {
		dev_err(priv->dev, "Invalid lpid %d\n", lpid);
		return -EINVAL;
	}

	ctp.lpid = lpid;
	gswip_ctp_port_get(priv, &ctp);

	reg_wbits(priv, SDMA_PCTRL(lpid), SDMA_PCTRL_PEN, DISABLE);
	regmap_write(priv->regmap, ETHSW_CTP_STARTID(lpid), 0);
	regmap_write(priv->regmap, ETHSW_CTP_ENDID(lpid), 0);

	bitmap_clear(priv->ctp_port_map, ctp.first_pid, ctp.num_port);

	return 0;
}

static int gswip_bridge_port_alloc(struct device *dev,
				   struct gswip_br_port_alloc *bp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct pce_tbl_prog pce_tbl = {0};
	int ret;

	bp->br_pid = find_first_zero_bit(priv->br_port_map, priv->num_br_port);
	if (bp->br_pid >= priv->num_br_port) {
		dev_err(priv->dev, "failed to alloc bridge port\n");
		return -EINVAL;
	}

	set_bit(bp->br_pid, priv->br_port_map);

	pce_tbl.id = PCE_IG_BRP_CFG;
	pce_tbl.addr = bp->br_pid;
	pce_tbl.val[4] = PCE_MAC_LIMIT_NUM |
			 FIELD_PREP(PCE_IGBGP_VAL4_BR_ID, bp->br_id);

	ret = gswip_pce_table_write(priv, &pce_tbl);
	if (ret) {
		clear_bit(bp->br_pid, priv->br_port_map);
		return ret;
	}

	return 0;
}

static int gswip_bridge_port_free(struct device *dev,
				  struct gswip_br_port_alloc *bp)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct pce_tbl_prog pce_tbl = {0};
	u16 br_pid;
	int ret;

	br_pid = bp->br_pid;
	if (br_pid >= priv->num_br_port) {
		dev_err(priv->dev, "brige port id %d >=%d\n",
			br_pid, priv->num_br_port);
		return -EINVAL;
	}

	if (!test_bit(br_pid, priv->br_port_map)) {
		dev_err(priv->dev, "bridge port id %d is not in used\n",
			br_pid);
		return -EINVAL;
	}

	pce_tbl.id = PCE_IG_BRP_CFG;
	pce_tbl.addr = br_pid;
	pce_tbl.val[4] = PCE_MAC_LIMIT_NUM;

	ret = gswip_pce_table_write(priv, &pce_tbl);
	if (ret)
		return ret;

	clear_bit(br_pid, priv->br_port_map);

	return 0;
}

static int gswip_bridge_alloc(struct device *dev, struct gswip_br_alloc *br)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);

	br->br_id = find_first_zero_bit(priv->br_map, priv->num_br);
	if (br->br_id >= priv->num_br) {
		dev_err(priv->dev, "failed to alloc bridge\n");
		return -EINVAL;
	}

	set_bit(br->br_id, priv->br_map);

	return 0;
}

static int gswip_bridge_free(struct device *dev, struct gswip_br_alloc *br)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct pce_tbl_prog pce_tbl = {0};
	u16 br_id;
	int ret;

	if (br->br_id >= priv->num_br) {
		dev_err(priv->dev, "bridge id %d >= %d\n",
			br->br_id, priv->num_br);
		return -EINVAL;
	}

	br_id = br->br_id;
	if (!test_bit(br_id, priv->br_map)) {
		dev_err(priv->dev, "bridge id %d not allocated\n", br_id);
		return 0;
	}

	pce_tbl.id = PCE_BR_CFG;
	pce_tbl.addr = br_id;
	pce_tbl.val[0] = PCE_MAC_LIMIT_NUM;

	ret = gswip_pce_table_write(priv, &pce_tbl);
	if (ret)
		return ret;

	clear_bit(br_id, priv->br_map);

	return 0;
}

/* Set queue for a logical port based on egress/ingress packet.
 * Then map the queue to an egress port.
 *  +----------------------+               +---------------------+
 *  | ingress logical port | -> queue/s -> | egress logical port |
 *  +----------------------+               +---------------------+
 */
static int gswip_qos_q_port_set(struct device *dev,
				struct gswip_qos_q_port *qport)
{
	struct gswip_core_priv *priv = dev_get_drvdata(dev);
	struct pce_tbl_prog pce_tbl = {0};
	struct bm_tbl_prog bm_tbl = {0};
	u16 eg_port, val;
	u8 qid;
	int ret;

	qid = qport->qid;

	if (qport->egress) {
		/* Egress packet always bypass PCE.
		 * Queue identifier is set in the SDMA_BYPASS register
		 * for each logical port.
		 */
		reg_r16(priv, SDMA_BYPASS(qport->lpid), &val);

		if (!qport->extration_en) {
			if (qport->q_map_mode == GSWIP_QOS_QMAP_SINGLE_MD)
				val |= FIELD_PREP(SDMA_BYPASS_MD, 1);
			else
				val |= FIELD_PREP(SDMA_BYPASS_MD, 0);

			val |= FIELD_PREP(SDMA_BYPASS_NMQID, qid);
		} else {
			val |= FIELD_PREP(SDMA_BYPASS_EXTQID, qid);
		}

		regmap_write(priv->regmap, SDMA_BYPASS(qport->lpid), val);
	} else {
		/* Ingress packet can bypass or not bypass PCE.
		 * Queue identifier is set in the Queue Mapping Table
		 * under PCE Table Programming. This table will be used
		 * for bypass/no bypass case.
		 */
		pce_tbl.id = PCE_Q_MAP;
		pce_tbl.addr = qport->tc_id;
		pce_tbl.addr |= FIELD_PREP(PCE_Q_MAP_EG_PID, qport->lpid);

		if (qport->en_ig_pce_bypass || qport->resv_port_mode) {
			pce_tbl.addr |= PCE_Q_MAP_IG_PORT_MODE;
		} else {
			if (qport->extration_en)
				pce_tbl.addr |= PCE_Q_MAP_LOCAL_EXTRACT;
		}

		pce_tbl.val[0] = qid;
		ret = gswip_pce_table_write(priv, &pce_tbl);
		if (ret)
			return ret;

		reg_wbits(priv, SDMA_BYPASS(qport->lpid),
			  SDMA_BYPASS_PCEBYP, qport->en_ig_pce_bypass);
	}

	/* Redirect port id for table is bit 4, and bit (2,0) */
	eg_port = FIELD_GET(BM_Q_MAP_VAL4_REDIR_PID, qport->redir_port_id);
	val = FIELD_GET(BM_Q_MAP_VAL4_REDIR_PID_MSB, qport->redir_port_id);
	eg_port |= FIELD_PREP(BM_Q_MAP_VAL4_REDIR_PID_BIT4, val);

	/* Map queue to an egress port. */
	bm_tbl.id = BM_Q_MAP;
	bm_tbl.val[0] = eg_port;
	bm_tbl.num_val = 1;
	bm_tbl.qmap.qid = qid;

	return gswip_bm_table_write(priv, &bm_tbl);
}

static const struct core_ctp_ops gswip_core_ctp_ops = {
	.alloc = gswip_ctp_port_alloc,
	.free = gswip_ctp_port_free,
};

static const struct core_br_port_ops gswip_core_br_port_ops = {
	.alloc = gswip_bridge_port_alloc,
	.free = gswip_bridge_port_free,
};

static const struct core_br_ops gswip_core_br_ops = {
	.alloc = gswip_bridge_alloc,
	.free = gswip_bridge_free,
};

static const struct core_qos_ops gswip_core_qos_ops = {
	.q_port_set = gswip_qos_q_port_set,
};

int gswip_core_setup_port_ops(struct gswip_core_priv *priv)
{
	struct core_ops *ops =  &priv->ops;

	ops->ctp_ops = &gswip_core_ctp_ops;
	ops->br_port_ops = &gswip_core_br_port_ops;
	ops->br_ops = &gswip_core_br_ops;
	ops->qos_ops = &gswip_core_qos_ops;

	return 0;
}
