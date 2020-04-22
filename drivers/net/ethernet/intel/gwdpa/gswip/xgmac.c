// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation. */

#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>

#include "mac_common.h"
#include "xgmac.h"

/**
 * xgmac_reg_rw - XGMAC register indirect read and write access.
 * @priv: mac private data.
 * @reg: register offset.
 * @val:
 *	read - pointer variable to read the value into.
 *	write - pointer variable to the write value.
 * @rd_wr: boolean value, true/false.
 *	false - read.
 *	true - write.
 */
static inline void xgmac_reg_rw(struct gswip_mac *priv, u16 reg, u32 *val,
				bool rd_wr)
{
	void __iomem *xgmac_ctrl_reg  = priv->sw + priv->xgmac_ctrl_reg;
	void __iomem *xgmac_data0_reg = priv->sw + priv->xgmac_data0_reg;
	void __iomem *xgmac_data1_reg = priv->sw + priv->xgmac_data1_reg;
	u32 access = XGMAC_REGACC_CTRL_ADDR_BAS;
	u32 retries = 2000;

	spin_lock_bh(&priv->irw_lock);
	access &= ~XGMAC_REGACC_CTRL_OPMOD_WR;
	if (rd_wr) {
		writel(FIELD_GET(MASK_HIGH, *val), xgmac_data1_reg);
		writel(FIELD_GET(MASK_LOW, *val), xgmac_data0_reg);
		access |= XGMAC_REGACC_CTRL_OPMOD_WR;
	}

	writel(access | reg, xgmac_ctrl_reg);

	do {
		if (!(readl(xgmac_ctrl_reg) & XGMAC_REGACC_CTRL_ADDR_BAS))
			break;
		cpu_relax();
	} while (--retries);
	if (!retries)
		dev_warn(priv->dev, "Xgmac register %s failed for Offset %x\n",
			 rd_wr ? "write" : "read", reg);

	if (!rd_wr)
		*val = FIELD_PREP(MASK_HIGH, readl(xgmac_data1_reg)) |
		       readl(xgmac_data0_reg);
	spin_unlock_bh(&priv->irw_lock);
}

void xgmac_get_hw_features(struct gswip_mac *priv)
{
	struct xgmac_hw_features *hw_feat = &priv->hw_feat;
	u32 mac_hfr0, mac_hfr1, mac_hfr2;

	xgmac_reg_rw(priv, MAC_HW_F0, &mac_hfr0, false);
	xgmac_reg_rw(priv, MAC_HW_F1, &mac_hfr1, false);
	xgmac_reg_rw(priv, MAC_HW_F2, &mac_hfr2, false);
	xgmac_reg_rw(priv, MAC_VER, &hw_feat->version, false);

	priv->ver = FIELD_GET(MAC_VER_USERVER, hw_feat->version);

	/* Hardware feature0 regiter*/
	hw_feat->gmii = FIELD_GET(MAC_HW_F0_GMIISEL, mac_hfr0);
	hw_feat->vlhash = FIELD_GET(MAC_HW_F0_VLHASH, mac_hfr0);
	hw_feat->sma = FIELD_GET(MAC_HW_F0_SMASEL, mac_hfr0);
	hw_feat->rwk = FIELD_GET(MAC_HW_F0_RWKSEL, mac_hfr0);
	hw_feat->mgk = FIELD_GET(MAC_HW_F0_MGKSEL, mac_hfr0);
	hw_feat->mmc = FIELD_GET(MAC_HW_F0_MMCSEL, mac_hfr0);
	hw_feat->aoe = FIELD_GET(MAC_HW_F0_ARPOFFSEL, mac_hfr0);
	hw_feat->ts = FIELD_GET(MAC_HW_F0_TSSEL, mac_hfr0);
	hw_feat->eee = FIELD_GET(MAC_HW_F0_EEESEL, mac_hfr0);
	hw_feat->tx_coe = FIELD_GET(MAC_HW_F0_TXCOESEL, mac_hfr0);
	hw_feat->rx_coe = FIELD_GET(MAC_HW_F0_RXCOESEL, mac_hfr0);
	hw_feat->addn_mac = FIELD_GET(MAC_HW_F0_ADDMACADRSEL, mac_hfr0);
	hw_feat->ts_src = FIELD_GET(MAC_HW_F0_TSSTSSEL, mac_hfr0);
	hw_feat->sa_vlan_ins = FIELD_GET(MAC_HW_F0_SAVLANINS, mac_hfr0);
	hw_feat->vxn = FIELD_GET(MAC_HW_F0_VXN, mac_hfr0);
	hw_feat->ediffc = FIELD_GET(MAC_HW_F0_EDIFFC, mac_hfr0);
	hw_feat->edma = FIELD_GET(MAC_HW_F0_EDMA, mac_hfr0);

	/* Hardware feature1 register*/
	hw_feat->rx_fifo_size = FIELD_GET(MAC_HW_F1_RXFIFOSIZE, mac_hfr1);
	hw_feat->tx_fifo_size = FIELD_GET(MAC_HW_F1_TXFIFOSIZE, mac_hfr1);
	hw_feat->osten = FIELD_GET(MAC_HW_F1_OSTEN, mac_hfr1);
	hw_feat->ptoen = FIELD_GET(MAC_HW_F1_PTOEN, mac_hfr1);
	hw_feat->adv_ts_hi = FIELD_GET(MAC_HW_F1_ADVTHWORD, mac_hfr1);
	hw_feat->dma_width = FIELD_GET(MAC_HW_F1_ADDR64, mac_hfr1);
	hw_feat->dcb = FIELD_GET(MAC_HW_F1_DCBEN, mac_hfr1);
	hw_feat->sph = FIELD_GET(MAC_HW_F1_SPHEN, mac_hfr1);
	hw_feat->tso = FIELD_GET(MAC_HW_F1_TSOEN, mac_hfr1);
	hw_feat->dma_debug = FIELD_GET(MAC_HW_F1_DBGMEMA, mac_hfr1);
	hw_feat->rss = FIELD_GET(MAC_HW_F1_RSSEN, mac_hfr1);
	hw_feat->tc_cnt = FIELD_GET(MAC_HW_F1_NUMTC, mac_hfr1);
	hw_feat->hash_table_size = FIELD_GET(MAC_HW_F1_HASHTBLSZ, mac_hfr1);
	hw_feat->l3l4_filter_num = FIELD_GET(MAC_HW_F1_L3L4FNUM, mac_hfr1);

	/* Hardware feature2 register*/
	hw_feat->rx_q_cnt = FIELD_GET(MAC_HW_F2_RXCHCNT, mac_hfr2);
	hw_feat->tx_q_cnt = FIELD_GET(MAC_HW_F2_TXQCNT, mac_hfr2);
	hw_feat->rx_ch_cnt = FIELD_GET(MAC_HW_F2_RXCHCNT, mac_hfr2);
	hw_feat->tx_ch_cnt = FIELD_GET(MAC_HW_F2_TXCHCNT, mac_hfr2);
	hw_feat->pps_out_num = FIELD_GET(MAC_HW_F2_PPSOUTNUM, mac_hfr2);
	hw_feat->aux_snap_num = FIELD_GET(MAC_HW_F2_AUXSNAPNUM, mac_hfr2);

	/* TC and Queue are zero based so increment to get the actual number */
	hw_feat->tc_cnt++;
	hw_feat->rx_q_cnt++;
	hw_feat->tx_q_cnt++;
}

void xgmac_init_priv(struct gswip_mac *priv)
{
	u8 mac_addr[6] = {0x00, 0x00, 0x94, 0x00, 0x00, 0x08};

	priv->mac_idx += MAC2;

	priv->xgmac_ctrl_reg = XGMAC_CTRL_REG(priv->mac_idx);
	priv->xgmac_data1_reg = XGMAC_DATA1_REG(priv->mac_idx);
	priv->xgmac_data0_reg = XGMAC_DATA0_REG(priv->mac_idx);

	/* Temp mac addr, Later eth driver will update */
	mac_addr[5] = priv->mac_idx;
	memcpy(priv->mac_addr, mac_addr, 6);

	priv->mtu = LGM_MAX_MTU;
	priv->promisc_mode = true;
	priv->all_mcast_mode = true;
}

void xgmac_set_mac_address(struct gswip_mac *priv, u8 *mac_addr)
{
	u32 mac_addr_hi, mac_addr_low;
	u32 val;

	mac_addr_hi = (mac_addr[5] << 8) | (mac_addr[4] << 0);
	mac_addr_low = (mac_addr[3] << 24) | (mac_addr[2] << 16) |
			(mac_addr[1] <<  8) | (mac_addr[0] <<  0);

	xgmac_reg_rw(priv, MAC_MACA0HR, &mac_addr_hi, true);

	xgmac_reg_rw(priv, MAC_MACA0LR, &val, false);
	if (val != mac_addr_low)
		xgmac_reg_rw(priv, MAC_MACA0LR, &mac_addr_low, true);
}

inline void xgmac_prep_pkt_jumbo(u32 mtu, u32 *mac_rcr, u32 *mac_tcr)
{
	if (mtu < XGMAC_MAX_GPSL) {	/* upto 9018 configuration */
		*mac_rcr |= MAC_RX_CFG_JE;
		*mac_rcr &= ~MAC_RX_CFG_WD & ~MAC_RX_CFG_GPSLCE;
		*mac_tcr &= ~MAC_TX_CFG_JD;

	} else {			/* upto 16K configuration */
		*mac_rcr &= ~MAC_RX_CFG_JE;
		*mac_rcr |= MAC_RX_CFG_WD | MAC_RX_CFG_GPSLCE |
			    FIELD_PREP(MAC_RX_CFG_GPSL,
				       XGMAC_MAX_SUPPORTED_MTU);
		*mac_tcr |= MAC_TX_CFG_JD;
	}
}

inline void xgmac_prep_pkt_standard(u32 *mac_rcr, u32 *mac_tcr)
{
	*mac_rcr &= ~MAC_RX_CFG_JE & ~MAC_RX_CFG_WD & ~MAC_RX_CFG_GPSLCE;
	*mac_tcr &= ~MAC_TX_CFG_JD;
}

void xgmac_config_pkt(struct gswip_mac *priv, u32 mtu)
{
	u32 mac_rcr, mac_tcr;

	xgmac_reg_rw(priv, MAC_RX_CFG, &mac_rcr, false);
	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, false);

	if (mtu > XGMAC_MAX_STD_PACKET)
		xgmac_prep_pkt_jumbo(mtu, &mac_rcr, &mac_tcr);
	else
		xgmac_prep_pkt_standard(&mac_rcr, &mac_tcr);

	xgmac_reg_rw(priv, MAC_RX_CFG, &mac_rcr, true);
	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, true);
}

void xgmac_config_packet_filter(struct gswip_mac *priv, u32 mode)
{
	u32 reg_val;

	xgmac_reg_rw(priv, MAC_PKT_FR, &reg_val, false);

	switch (mode) {
	case PROMISC:
		reg_val &= ~MAC_PKT_FR_PR;
		reg_val |= FIELD_PREP(MAC_PKT_FR_PR, priv->promisc_mode);
		break;

	case PASS_ALL_MULTICAST:
		reg_val &= ~MAC_PKT_FR_PM;
		reg_val |= FIELD_PREP(MAC_PKT_FR_PM, priv->all_mcast_mode);
		break;

	default:
		reg_val &= ~MAC_PKT_FR_PR & ~MAC_PKT_FR_PM;
		reg_val |= FIELD_PREP(MAC_PKT_FR_PR, priv->promisc_mode) |
			   FIELD_PREP(MAC_PKT_FR_PM, priv->all_mcast_mode);
	}

	xgmac_reg_rw(priv, MAC_PKT_FR, &reg_val, true);
}

void xgmac_tx_flow_ctl(struct gswip_mac *priv, u32 pause_time, u32 mode)
{
	u32 reg_val = 0;

	xgmac_reg_rw(priv, MAC_TX_FCR, &reg_val, false);

	switch (mode) {
	case XGMAC_FC_EN:
		/* enable tx flow control */
		reg_val |= MAC_TX_FCR_TFE;

		/* Set pause time */
		reg_val &= ~MAC_TX_FCR_PT;
		reg_val |= FIELD_PREP(MAC_TX_FCR_PT, pause_time);
		break;

	case XGMAC_FC_DIS:
		reg_val &= ~MAC_TX_FCR_TFE;
		break;
	}

	xgmac_reg_rw(priv, MAC_TX_FCR, &reg_val, true);
}

void xgmac_rx_flow_ctl(struct gswip_mac *priv, u32 mode)
{
	u32 reg_val = 0;

	xgmac_reg_rw(priv, MAC_RX_FCR, &reg_val, false);

	switch (mode) {
	case XGMAC_FC_EN:
		/* rx fc enable */
		reg_val |= MAC_RX_FCR_RFE;
		reg_val &= ~MAC_RX_FCR_PFCE;
		break;

	case XGMAC_FC_DIS:
		reg_val &= ~MAC_RX_FCR_RFE;
		break;
	}

	xgmac_reg_rw(priv, MAC_RX_FCR, &reg_val, true);
}

int xgmac_set_mac_lpitx(struct gswip_mac *priv, u32 val)
{
	u32 lpiate;

	xgmac_reg_rw(priv, MAC_LPI_CSR, &lpiate, false);

	if (FIELD_GET(MAC_LPI_CSR_LPIATE, lpiate) != val) {
		lpiate &= ~MAC_LPI_CSR_LPIATE;
		lpiate |= FIELD_PREP(MAC_LPI_CSR_LPIATE, val);
	}

	if (FIELD_GET(MAC_LPI_CSR_LPITXA, lpiate) != val) {
		lpiate &= ~MAC_LPI_CSR_LPITXA;
		lpiate |= FIELD_PREP(MAC_LPI_CSR_LPITXA, val);
	}

	xgmac_reg_rw(priv, MAC_LPI_CSR, &lpiate, true);

	return 0;
}

int xgmac_enable(struct gswip_mac *priv)
{
	u32 mac_tcr, mac_rcr, mac_pfr;

	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, false);
	xgmac_reg_rw(priv, MAC_RX_CFG, &mac_rcr, false);
	xgmac_reg_rw(priv, MAC_PKT_FR, &mac_pfr, false);

	/* Enable MAC Tx */
	if (!FIELD_GET(MAC_TX_CFG_TE, mac_tcr)) {
		mac_tcr |= MAC_TX_CFG_TE;
		xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, true);
	}

	/* Enable MAC Rx */
	if (!FIELD_GET(MAC_RX_CFG_RE, mac_rcr)) {
		mac_rcr |= MAC_RX_CFG_RE;
		xgmac_reg_rw(priv, MAC_RX_CFG, &mac_rcr, true);
	}

	/* Enable MAC Filter Rx All */
	if (!FIELD_GET(MAC_PKT_FR_RA, mac_pfr)) {
		mac_pfr |= MAC_PKT_FR_RA;
		xgmac_reg_rw(priv, MAC_PKT_FR, &mac_pfr, true);
	}

	return 0;
}

int xgmac_disable(struct gswip_mac *priv)
{
	u32 mac_tcr, mac_rcr, mac_pfr;

	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, false);
	xgmac_reg_rw(priv, MAC_RX_CFG, &mac_rcr, false);
	xgmac_reg_rw(priv, MAC_PKT_FR, &mac_pfr, false);

	/* Disable MAC Tx */
	if (FIELD_GET(MAC_TX_CFG_TE, mac_tcr) != 0) {
		mac_tcr &= ~MAC_TX_CFG_TE;
		xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, true);
	}

	/* Disable MAC Rx */
	if (FIELD_GET(MAC_RX_CFG_RE, mac_rcr) != 0) {
		mac_rcr &= ~MAC_RX_CFG_RE;
		xgmac_reg_rw(priv, MAC_RX_CFG, &mac_rcr, true);
	}

	/* Disable MAC Filter Rx All */
	if (FIELD_GET(MAC_PKT_FR_RA, mac_pfr) != 0) {
		mac_pfr &= ~MAC_PKT_FR_RA;
		xgmac_reg_rw(priv, MAC_PKT_FR, &mac_pfr, true);
	}

	return 0;
}

int xgmac_pause_frame_filter(struct gswip_mac *priv, u32 val)
{
	u32 mac_pfr;

	xgmac_reg_rw(priv, MAC_PKT_FR, &mac_pfr, false);

	if (FIELD_GET(MAC_PKT_FR_PCF, mac_pfr) != val) {
		/* Pause filtering */
		mac_pfr &= ~MAC_PKT_FR_PCF;
		mac_pfr |= FIELD_PREP(MAC_PKT_FR_PCF, val);
	}

	/* The Receiver module passes only those packets to the application
	 * that pass the SA or DA address filter.
	 */
	if (FIELD_GET(MAC_PKT_FR_RA, mac_pfr) == 1)
		mac_pfr &= ~MAC_PKT_FR_RA;

	xgmac_reg_rw(priv, MAC_PKT_FR, &mac_pfr, true);

	return 0;
}

int xgmac_set_extcfg(struct gswip_mac *priv, u32 val)
{
	u32 mac_extcfg;

	xgmac_reg_rw(priv, MAC_EXTCFG, &mac_extcfg, false);

	if (FIELD_GET(MAC_EXTCFG_SBDIOEN, mac_extcfg) != val) {
		mac_extcfg &= ~MAC_EXTCFG_SBDIOEN;
		mac_extcfg |= FIELD_PREP(MAC_EXTCFG_SBDIOEN, val);
		xgmac_reg_rw(priv, MAC_EXTCFG, &mac_extcfg, true);
	}

	return 0;
}

int xgmac_set_xgmii_speed(struct gswip_mac *priv)
{
	u32 mac_tcr;

	xgmac_disable(priv);
	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, false);

	if (FIELD_GET(MAC_TX_CFG_USS, mac_tcr) != 0)
		mac_tcr &= ~MAC_TX_CFG_USS;

	if (FIELD_GET(MAC_TX_CFG_SS, mac_tcr) != 0)
		mac_tcr &= ~MAC_TX_CFG_SS;

	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, true);
	xgmac_enable(priv);

	return 0;
}

int xgmac_set_gmii_2500_speed(struct gswip_mac *priv)
{
	u32 mac_tcr;

	xgmac_disable(priv);
	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, false);

	if (FIELD_GET(MAC_TX_CFG_USS, mac_tcr) != 0)
		mac_tcr &= ~MAC_TX_CFG_USS;

	if (FIELD_GET(MAC_TX_CFG_SS, mac_tcr) != 0x2) {
		mac_tcr &= ~MAC_TX_CFG_SS;
		mac_tcr |= FIELD_PREP(MAC_TX_CFG_SS, 0x2);
	}

	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, true);
	xgmac_enable(priv);

	return 0;
}

int xgmac_set_xgmii_2500_speed(struct gswip_mac *priv)
{
	u32 mac_tcr;

	xgmac_disable(priv);
	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, false);

	if (FIELD_GET(MAC_TX_CFG_USS, mac_tcr) != 1)
		mac_tcr |= MAC_TX_CFG_USS;

	if (FIELD_GET(MAC_TX_CFG_SS, mac_tcr) != 0x2) {
		mac_tcr &= ~MAC_TX_CFG_SS;
		mac_tcr |= FIELD_PREP(MAC_TX_CFG_SS, 0x2);
	}

	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, true);
	xgmac_enable(priv);

	return 0;
}

int xgmac_set_gmii_speed(struct gswip_mac *priv)
{
	u32 mac_tcr;

	xgmac_disable(priv);
	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, false);

	if (FIELD_GET(MAC_TX_CFG_USS, mac_tcr) != 0)
		mac_tcr &= MAC_TX_CFG_USS;

	if (FIELD_GET(MAC_TX_CFG_SS, mac_tcr) != 0x3) {
		mac_tcr &= ~MAC_TX_CFG_SS;
		mac_tcr |= FIELD_PREP(MAC_TX_CFG_SS, 0x3);
	}

	xgmac_reg_rw(priv, MAC_TX_CFG, &mac_tcr, true);
	xgmac_enable(priv);

	return 0;
}

int xgmac_mdio_set_clause(struct gswip_mac *priv, u32 clause, u32 phy_id)
{
	u32 mdio_c22p = 0;

	xgmac_reg_rw(priv, MDIO_C22P, &mdio_c22p, false);

	if (clause == MDIO_CLAUSE22)
		mdio_c22p |= MDIO_C22P_PORT(phy_id);
	else
		mdio_c22p &= ~MDIO_C22P_PORT(phy_id);

	/* Select port 0, 1, 2 and 3 as Clause 22/45 ports */
	xgmac_reg_rw(priv, MDIO_C22P, &mdio_c22p, true);

	return 0;
}

static int xgmac_mdio_single_wr(struct gswip_mac *priv, u32 dev_adr,
				u32 phy_id, u32 phy_reg, u32 phy_reg_data)
{
	u32 mdio_sccdr, mdio_scar;
	u32 retries = 100;

	/* wait for any previous MDIO read/write operation to complete */
	/* Poll */
	do {
		xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, false);
		if (!FIELD_GET(MDIO_SCCDR_BUSY, mdio_sccdr))
			break;
		cpu_relax();
	} while (--retries);
	if (!retries) {
		dev_err(priv->dev, "Xgmac MDIO rd/wr operation failed\n");
		return -ETIMEDOUT;
	}

	xgmac_reg_rw(priv, MDIO_SCAR, &mdio_scar, false);
	mdio_scar &= ~MDIO_SCAR_DA & ~MDIO_SCAR_PA & ~MDIO_SCAR_RA;
	mdio_scar |= FIELD_PREP(MDIO_SCAR_DA, dev_adr);
	mdio_scar |= FIELD_PREP(MDIO_SCAR_PA, phy_id);
	mdio_scar |= FIELD_PREP(MDIO_SCAR_RA, phy_reg);
	xgmac_reg_rw(priv, MDIO_SCAR, &mdio_scar, true);

	xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, false);
	mdio_sccdr &= ~MDIO_SCCDR_SDATA & ~MDIO_SCCDR_CMD;
	mdio_sccdr |= FIELD_PREP(MDIO_SCCDR_SDATA, phy_reg_data);
	mdio_sccdr |= MDIO_SCCDR_BUSY;
	mdio_sccdr &= ~MDIO_SCCDR_SADDR;
	mdio_sccdr |= FIELD_PREP(MDIO_SCCDR_CMD, 1);
	xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, true);

	retries = 100;
	/* wait for MDIO read operation to complete */
	/* Poll */
	do {
		xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, false);
		if (!FIELD_GET(MDIO_SCCDR_BUSY, mdio_sccdr))
			break;
		cpu_relax();
	} while (--retries);
	if (!retries) {
		dev_err(priv->dev, "Xgmac MDIO rd/wr operation failed\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int xgmac_mdio_single_rd(struct gswip_mac *priv, u32 dev_adr,
				u32 phy_id, u32 phy_reg)
{
	u32 mdio_sccdr, mdio_scar;
	u32 retries = 100;
	int phy_reg_data;

	/* wait for any previous MDIO read/write operation to complete */
	/* Poll */
	do {
		xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, false);
		if (!FIELD_GET(MDIO_SCCDR_BUSY, mdio_sccdr))
			break;
		cpu_relax();
	} while (--retries);
	if (!retries) {
		dev_err(priv->dev, "Xgmac MDIO rd/wr operation failed\n");
		return -ETIMEDOUT;
	}

	/* initiate the MDIO read operation by updating desired bits
	 * PA - phy address/id (0 - 31)
	 * RA - phy register offset
	 */

	xgmac_reg_rw(priv, MDIO_SCAR, &mdio_scar, false);
	mdio_scar &= ~MDIO_SCAR_DA & ~MDIO_SCAR_PA & ~MDIO_SCAR_RA;
	mdio_scar |= FIELD_PREP(MDIO_SCAR_DA, dev_adr);
	mdio_scar |= FIELD_PREP(MDIO_SCAR_PA, phy_id);
	mdio_scar |= FIELD_PREP(MDIO_SCAR_RA, phy_reg);
	xgmac_reg_rw(priv, MDIO_SCAR, &mdio_scar, true);

	xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, false);
	mdio_sccdr &= ~MDIO_SCCDR_CMD & ~MDIO_SCCDR_SDATA;
	mdio_sccdr |= MDIO_SCCDR_BUSY;
	mdio_sccdr &= ~MDIO_SCCDR_SADDR;
	mdio_sccdr |= FIELD_PREP(MDIO_SCCDR_CMD, 3);
	mdio_sccdr |= FIELD_PREP(MDIO_SCCDR_SDATA, 0);
	xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, true);

	retries = 100;
	/* wait for MDIO read operation to complete */
	/* Poll */
	do {
		xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, false);
		if (!FIELD_GET(MDIO_SCCDR_BUSY, mdio_sccdr))
			break;
		cpu_relax();
	} while (--retries);
	if (!retries) {
		dev_err(priv->dev, "Xgmac MDIO rd/wr operation failed\n");
		return -ETIMEDOUT;
	}

	/* read the data */
	xgmac_reg_rw(priv, MDIO_SCCDR, &mdio_sccdr, false);
	phy_reg_data = FIELD_GET(MDIO_SCCDR_SDATA, mdio_sccdr);

	return phy_reg_data;
}

static int xgmac_mdio_read(struct mii_bus *bus, int phyadr, int phyreg)
{
	struct gswip_mac *priv = bus->priv;

	return xgmac_mdio_single_rd(priv, 0, phyadr, phyreg);
}

static int xgmac_mdio_write(struct mii_bus *bus, int phyadr, int phyreg,
			    u16 phydata)
{
	struct gswip_mac *priv = bus->priv;

	return xgmac_mdio_single_wr(priv, 0, phyadr, phyreg, phydata);
}

int xgmac_mdio_register(struct gswip_mac *priv)
{
	struct device_node *mdio_np;
	struct mii_bus *bus;
	int ret;

	mdio_np = of_get_child_by_name(priv->dev->of_node, "mdio");
	if (!mdio_np)
		return -ENOLINK;

	bus = mdiobus_alloc();
	if (!bus)
		return -ENOMEM;

	bus->name = "xgmac_phy";
	bus->read = xgmac_mdio_read;
	bus->write = xgmac_mdio_write;
	bus->reset = NULL;
	snprintf(bus->id, MII_BUS_ID_SIZE, "%s-%x", bus->name, priv->mac_idx);
	bus->priv = priv;
	bus->parent = priv->dev;

	/* At this moment gphy is not yet up (firmware not yet loaded), so we
	 * avoid auto mdio scan.
	 */
	bus->phy_mask = 0xFFFFFFFF;

	ret = of_mdiobus_register(bus, mdio_np);
	if (ret) {
		mdiobus_free(bus);
		return ret;
	}

	priv->mii = bus;
	dev_info(priv->dev, "XGMAC %d: MDIO register Successful\n",
		 priv->mac_idx);

	return 0;
}
