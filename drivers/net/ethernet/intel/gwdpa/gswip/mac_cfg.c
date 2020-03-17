// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation. */

#include <linux/bitfield.h>
#include <linux/ethtool.h>
#include "mac_common.h"

static int mac_speed_to_val(u32 speed)
{
	int val;

	switch (speed) {
	case SPEED_10M:
		val = SPEED_10;
		break;
	case SPEED_100M:
		val = SPEED_100;
		break;
	case SPEED_1G:
		val = SPEED_1000;
		break;
	case SPEED_10G:
		val = SPEED_10000;
		break;
	case SPEED_2G5:
		val = SPEED_2500;
		break;
	case SPEED_5G:
		val = SPEED_5000;
		break;
	default:
		val = SPEED_UNKNOWN;
	}

	return val;
}

static int mac_get_speed(struct device *dev)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);
	u32 mac_speed;

	spin_lock_bh(&priv->mac_lock);
	mac_speed = sw_get_speed(priv);
	spin_unlock_bh(&priv->mac_lock);

	return mac_speed_to_val(mac_speed);
}

static int mac_set_physpeed(struct gswip_mac *priv, u32 phy_speed)
{
	spin_lock_bh(&priv->mac_lock);
	xgmac_set_extcfg(priv, 1);

	switch (phy_speed) {
	default:
	case SPEED_MAC_AUTO:
		sw_set_speed(priv, SPEED_AUTO);
		break;

	case SPEED_XGMAC_10G:
		xgmac_set_xgmii_speed(priv);
		sw_set_speed(priv, SPEED_10G);
		break;

	case SPEED_GMII_25G:
		sw_set_speed(priv, SPEED_2G5);
		sw_set_2G5_intf(priv, XGMAC_GMII);
		xgmac_set_gmii_2500_speed(priv);
		break;

	case SPEED_XGMII_25G:
		sw_set_speed(priv, SPEED_2G5);
		sw_set_2G5_intf(priv, XGMAC_XGMII);
		xgmac_set_xgmii_2500_speed(priv);
		break;

	case SPEED_XGMAC_1G:
		sw_set_speed(priv, SPEED_1G);
		sw_set_1g_intf(priv, XGMAC_GMII);
		xgmac_set_gmii_speed(priv);
		xgmac_set_extcfg(priv, 1);
		break;

	case SPEED_XGMAC_10M:
		sw_set_speed(priv, SPEED_10M);
		/* FALLTHRU */
	case SPEED_XGMAC_100M:
		if (phy_speed != SPEED_XGMAC_10M)
			sw_set_speed(priv, SPEED_100M);

		sw_set_fe_intf(priv, XGMAC_GMII);
		sw_set_1g_intf(priv, XGMAC_GMII);
		xgmac_set_gmii_speed(priv);
		break;

	case SPEED_LMAC_10M:
		sw_set_speed(priv, SPEED_10M);
		/* FALLTHRU */
	case SPEED_LMAC_100M:
		if (phy_speed != SPEED_LMAC_10M)
			sw_set_speed(priv, SPEED_100M);

		sw_set_fe_intf(priv, LMAC_MII);
		lmac_set_intf_mode(priv, 1);
		break;

	case SPEED_LMAC_1G:
		sw_set_speed(priv, SPEED_1G);
		sw_set_1g_intf(priv, LMAC_GMII);
		lmac_set_intf_mode(priv, 2);
		break;
	}
	spin_unlock_bh(&priv->mac_lock);

	return 0;
}

static int mac_set_duplex(struct gswip_mac *priv, u32 mode)
{
	u32 val;

	spin_lock_bh(&priv->mac_lock);
	switch (mode) {
	default:
	case GSW_DUPLEX_AUTO:
		val = PHY_MODE_FDUP_AUTO;
		break;
	case GSW_DUPLEX_HALF:
		val = PHY_MODE_FDUP_HD;
		break;
	case GSW_DUPLEX_FULL:
		val = PHY_MODE_FDUP_FD;
		break;
	}

	sw_set_duplex_mode(priv, val);
	lmac_set_duplex_mode(priv, val);
	spin_unlock_bh(&priv->mac_lock);

	return 0;
}

static int mac_get_duplex(struct device *dev)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);
	int val;

	spin_lock_bh(&priv->mac_lock);
	val = sw_get_duplex_mode(priv);
	spin_unlock_bh(&priv->mac_lock);

	return val;
}

static int mac_get_linksts(struct device *dev)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);
	int linksts;

	spin_lock_bh(&priv->mac_lock);
	linksts = sw_get_linkstatus(priv);
	spin_unlock_bh(&priv->mac_lock);

	return linksts;
}

static int mac_set_linksts(struct gswip_mac *priv, u32 mode)
{
	u8 val;

	spin_lock_bh(&priv->mac_lock);
	switch (mode) {
	default:
	case LINK_AUTO:
		val = PHY_MODE_LINKST_AUTO;
		break;

	case LINK_UP:
		val = PHY_MODE_LINKST_UP;
		break;

	case LINK_DOWN:
		val = PHY_MODE_LINKST_DOWN;
		break;
	}
	sw_set_linkstatus(priv, val);
	spin_unlock_bh(&priv->mac_lock);

	return 0;
}

static int mac_set_flowctrl(struct device *dev, u32 val)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);

	if (val >= FC_INVALID)
		return -EINVAL;

	spin_lock_bh(&priv->mac_lock);
	lmac_set_flowcon_mode(priv, val);

	switch (val) {
	default:
	case FC_AUTO:
		xgmac_tx_flow_ctl(priv, priv->pause_time, XGMAC_FC_EN);
		xgmac_rx_flow_ctl(priv, XGMAC_FC_EN);
		sw_set_flowctrl(priv, PHY_MODE_FCON_AUTO, FCONRX);
		sw_set_flowctrl(priv, PHY_MODE_FCON_AUTO, FCONTX);
		break;

	case FC_RX:
		/* Disable TX in XGMAC and GSWSS */
		xgmac_tx_flow_ctl(priv, priv->pause_time, XGMAC_FC_DIS);
		sw_set_flowctrl(priv, PHY_MODE_FCON_DIS, FCONTX);

		/* Enable RX in XGMAC and GSWSS */
		xgmac_rx_flow_ctl(priv, XGMAC_FC_EN);

		sw_set_flowctrl(priv, PHY_MODE_FCON_EN, FCONRX);
		break;

	case FC_TX:
		/* Disable RX in XGMAC and GSWSS */
		xgmac_rx_flow_ctl(priv, XGMAC_FC_DIS);
		sw_set_flowctrl(priv, PHY_MODE_FCON_DIS, FCONTX);

		/* Enable TX in XGMAC and GSWSS */
		xgmac_tx_flow_ctl(priv, priv->pause_time, XGMAC_FC_EN);
		sw_set_flowctrl(priv, PHY_MODE_FCON_EN, FCONTX);
		break;

	case FC_RXTX:
		xgmac_tx_flow_ctl(priv, priv->pause_time, XGMAC_FC_EN);
		xgmac_rx_flow_ctl(priv, XGMAC_FC_EN);
		sw_set_flowctrl(priv, PHY_MODE_FCON_EN, FCONRX);
		sw_set_flowctrl(priv, PHY_MODE_FCON_EN, FCONTX);
		break;

	case FC_DIS:
		xgmac_tx_flow_ctl(priv, priv->pause_time, XGMAC_FC_DIS);
		xgmac_rx_flow_ctl(priv, XGMAC_FC_DIS);
		sw_set_flowctrl(priv, PHY_MODE_FCON_DIS, FCONRX);
		sw_set_flowctrl(priv, PHY_MODE_FCON_EN, FCONTX);
		break;
	}
	spin_unlock_bh(&priv->mac_lock);

	return 0;
}

inline int get_2G5_intf(struct gswip_mac *priv)
{
	u32 mac_if_cfg, macif;
	int ret;

	mac_if_cfg = sw_read(priv, MAC_IF_CFG_REG(priv->mac_idx));
	macif = FIELD_PREP(MAC_IF_CFG_CFG2G5, mac_if_cfg);

	if (macif == 0)
		ret = XGMAC_GMII;
	else if (macif == 1)
		ret = XGMAC_XGMII;
	else
		ret = -EINVAL;

	return ret;
}

inline int get_1g_intf(struct gswip_mac *priv)
{
	u32 mac_if_cfg, macif;
	int ret;

	mac_if_cfg = sw_read(priv, MAC_IF_CFG_REG(priv->mac_idx));
	macif = FIELD_GET(MAC_IF_CFG_CFG1G, mac_if_cfg);

	if (macif == 0)
		ret = LMAC_GMII;
	else if (macif == 1)
		ret = XGMAC_GMII;
	else
		ret = -EINVAL;

	return ret;
}

inline int get_fe_intf(struct gswip_mac *priv)
{
	u32 mac_if_cfg, macif;
	int ret;

	mac_if_cfg = sw_read(priv, MAC_IF_CFG_REG(priv->mac_idx));
	macif = FIELD_GET(MAC_IF_CFG_CFGFE, mac_if_cfg);

	if (macif == 0)
		ret = LMAC_MII;
	else if (macif == 1)
		ret = XGMAC_GMII;
	else
		ret = -EINVAL;

	return ret;
}

static int mac_get_mii_interface(struct device *dev)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);
	int intf, ret;
	u8 mac_speed;

	spin_lock_bh(&priv->mac_lock);
	mac_speed = sw_get_speed(priv);
	switch (mac_speed) {
	case SPEED_10M:
	case SPEED_100M:
		intf = get_fe_intf(priv);
		if (intf == XGMAC_GMII)
			ret = GSW_PORT_HW_GMII;
		else if (intf == LMAC_MII)
			ret = GSW_PORT_HW_MII;
		else
			ret = -EINVAL;
		break;

	case SPEED_1G:
		intf = get_1g_intf(priv);
		if (intf == LMAC_GMII || intf == XGMAC_GMII)
			ret = GSW_PORT_HW_GMII;
		else
			ret = -EINVAL;
		break;

	case SPEED_2G5:
		intf = get_2G5_intf(priv);
		if (intf == XGMAC_GMII)
			ret = GSW_PORT_HW_GMII;
		else if (intf == XGMAC_XGMII)
			ret = GSW_PORT_HW_XGMII;
		else
			ret = -EINVAL;
		break;

	case SPEED_10G:
		ret = GSW_PORT_HW_XGMII;
		break;

	case SPEED_AUTO:
		ret = GSW_PORT_HW_XGMII;
		break;

	default:
		ret = -EINVAL;
	}
	spin_unlock_bh(&priv->mac_lock);

	return ret;
}

inline u32 set_mii_if_fe(u32 mode)
{
	u32 val = 0;

	switch (mode) {
	default:
	case LMAC_MII:
		val &= ~MAC_IF_CFG_CFGFE;
		break;

	case XGMAC_GMII:
		val |= MAC_IF_CFG_CFGFE;
		break;
	}

	return val;
}

inline u32 set_mii_if_1g(u32 mode)
{
	u32 val = 0;

	switch (mode) {
	default:
	case LMAC_GMII:
		val &= ~MAC_IF_CFG_CFG1G;
		break;

	case XGMAC_GMII:
		val |= MAC_IF_CFG_CFG1G;
		break;
	}

	return val;
}

inline u32 set_mii_if_2G5(u32 mode)
{
	u32 val = 0;

	switch (mode) {
	default:
	case XGMAC_GMII:
		val &= ~MAC_IF_CFG_CFG2G5;
		break;

	case XGMAC_XGMII:
		val |= MAC_IF_CFG_CFG2G5;
		break;
	}

	return val;
}

static int mac_set_mii_interface(struct device *dev, u32 mii_mode)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);
	u32 reg_val = 0;

	spin_lock_bh(&priv->mac_lock);
	reg_val = sw_read(priv, MAC_IF_CFG_REG(priv->mac_idx));

	/* Default modes...
	 *		2.5G -	XGMAC_GMII
	 *		1G   -	LMAC_GMII
	 *		FE   -	LMAC_MII
	 */
	switch (mii_mode) {
	default:
	case GSW_PORT_HW_XGMII:
		reg_val |= set_mii_if_2G5(XGMAC_XGMII);
		break;

	case GSW_PORT_HW_GMII:
		reg_val |= set_mii_if_1g(XGMAC_GMII);
		reg_val |= set_mii_if_fe(XGMAC_GMII);
		reg_val |= set_mii_if_2G5(XGMAC_GMII);
		break;

	case GSW_PORT_HW_MII:
		reg_val |= set_mii_if_fe(LMAC_MII);
		break;
	}

	sw_write(priv, MAC_IF_CFG_REG(priv->mac_idx), reg_val);
	spin_unlock_bh(&priv->mac_lock);

	return 0;
}

static u32 mac_get_mtu(struct device *dev)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);
	u32 val;

	spin_lock_bh(&priv->mac_lock);
	val = sw_mac_get_mtu(priv);
	spin_unlock_bh(&priv->mac_lock);

	return val;
}

static int mac_set_mtu(struct device *dev, u32 mtu)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);

	if (mtu > LGM_MAX_MTU)
		return -EINVAL;

	spin_lock_bh(&priv->mac_lock);
	sw_mac_set_mtu(priv, mtu);
	xgmac_config_pkt(priv, mtu);
	spin_unlock_bh(&priv->mac_lock);

	return 0;
}

static int mac_init(struct device *dev)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);

	xgmac_set_mac_address(priv, priv->mac_addr);
	mac_set_mtu(dev, priv->mtu);
	xgmac_config_packet_filter(priv, PROMISC);
	xgmac_config_packet_filter(priv, PASS_ALL_MULTICAST);
	mac_set_mii_interface(dev, GSW_PORT_HW_GMII);
	mac_set_flowctrl(dev, FC_RXTX);
	mac_set_linksts(priv, LINK_UP);
	mac_set_duplex(priv, GSW_DUPLEX_FULL);
	xgmac_set_mac_lpitx(priv, LPITX_EN);
	mac_set_physpeed(priv, SPEED_XGMAC_10G);
	/* Enable XMAC Tx/Rx */
	xgmac_enable(priv);
	xgmac_pause_frame_filter(priv, 1);
	sw_set_eee_cap(priv, ANEG_EEE_CAP_OFF);
	sw_set_mac_rxfcs_op(priv, MAC_OP_CFG_RX_FCS_M3);
	xgmac_mdio_set_clause(priv, MDIO_CLAUSE22, (priv->mac_idx - MAC2));
	xgmac_mdio_register(priv);

	return 0;
}

static const struct gsw_mac_ops lgm_mac_ops = {
	.init		= mac_init,
	.set_mtu	= mac_set_mtu,
	.get_mtu	= mac_get_mtu,
	.set_mii_if	= mac_set_mii_interface,
	.get_mii_if	= mac_get_mii_interface,
	.set_flowctrl	= mac_set_flowctrl,
	.get_link_sts	= mac_get_linksts,
	.get_duplex	= mac_get_duplex,
	.get_speed	= mac_get_speed,
};

static const struct gsw_adap_ops lgm_adap_ops = {
	.sw_core_enable = sw_core_enable,
};

void mac_init_ops(struct device *dev)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);

	priv->mac_ops = &lgm_mac_ops;
	priv->adap_ops = &lgm_adap_ops;
}
