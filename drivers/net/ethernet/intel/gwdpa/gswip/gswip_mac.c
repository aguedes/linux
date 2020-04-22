// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation. */

#include <linux/bitfield.h>
#include <linux/types.h>

#include "mac_common.h"

int sw_core_enable(struct device *dev, u32 val)
{
	struct gswip_mac *priv = dev_get_drvdata(dev);
	u32 reg;

	spin_lock_bh(&priv->sw_lock);
	reg = sw_read(priv, GSWIP_CFG);
	reg &= ~GSWIP_CFG_CORE_SE_EN;
	reg |= FIELD_PREP(GSWIP_CFG_CORE_SE_EN, val);
	sw_write(priv, GSWIP_CFG, reg);
	spin_unlock_bh(&priv->sw_lock);

	return 0;
}

int sw_set_mac_rxfcs_op(struct gswip_mac *priv, u32 val)
{
	u32 mac_op_cfg;

	mac_op_cfg = sw_read(priv, MAC_OP_CFG_REG(priv->mac_idx));
	if (FIELD_GET(MAC_OP_CFG_RX_FCS, mac_op_cfg) != val) {
		mac_op_cfg &= ~MAC_OP_CFG_RX_FCS;
		mac_op_cfg |= FIELD_PREP(MAC_OP_CFG_RX_FCS, val);
		sw_write(priv, MAC_OP_CFG_REG(priv->mac_idx), mac_op_cfg);
	}

	return 0;
}

int sw_set_eee_cap(struct gswip_mac *priv, u32 val)
{
	u32 aneg_eee;

	aneg_eee = sw_read(priv, ANEG_EEE_REG(priv->mac_idx));
	aneg_eee &= ~ANEG_EEE_CAP;
	aneg_eee |= FIELD_PREP(ANEG_EEE_CAP, val);
	sw_write(priv, ANEG_EEE_REG(priv->mac_idx), aneg_eee);

	return 0;
}

int sw_set_fe_intf(struct gswip_mac *priv, u32 macif)
{
	u32 mac_if_cfg;

	mac_if_cfg = sw_read(priv, MAC_IF_CFG_REG(priv->mac_idx));

	if (macif == LMAC_MII)
		mac_if_cfg &= ~MAC_IF_CFG_CFGFE;
	else
		mac_if_cfg |= MAC_IF_CFG_CFGFE;

	sw_write(priv, MAC_IF_CFG_REG(priv->mac_idx), mac_if_cfg);

	return 0;
}

int sw_set_1g_intf(struct gswip_mac *priv, u32 macif)
{
	u32 mac_if_cfg;

	mac_if_cfg = sw_read(priv, MAC_IF_CFG_REG(priv->mac_idx));

	if (macif == LMAC_GMII)
		mac_if_cfg &= ~MAC_IF_CFG_CFG1G;
	else
		mac_if_cfg |= MAC_IF_CFG_CFG1G;

	sw_write(priv, MAC_IF_CFG_REG(priv->mac_idx), mac_if_cfg);

	return 0;
}

int sw_set_2G5_intf(struct gswip_mac *priv, u32 macif)
{
	u32 mac_if_cfg;

	mac_if_cfg = sw_read(priv, MAC_IF_CFG_REG(priv->mac_idx));

	if (macif == XGMAC_GMII)
		mac_if_cfg &= ~MAC_IF_CFG_CFG2G5;
	else
		mac_if_cfg |= MAC_IF_CFG_CFG2G5;

	sw_write(priv, MAC_IF_CFG_REG(priv->mac_idx), mac_if_cfg);

	return 0;
}

int sw_set_speed(struct gswip_mac *priv, u8 speed)
{
	u16 phy_mode = 0;
	u8 speed_msb = 0, speed_lsb = 0;

	phy_mode = sw_read(priv, PHY_MODE_REG(priv->mac_idx));

	/* clear first */
	phy_mode &= ~PHY_MODE_SPEED_MSB & ~PHY_MODE_SPEED_LSB;

	speed_msb = FIELD_GET(SPEED_MSB, speed);
	speed_lsb = FIELD_GET(SPEED_LSB, speed);
	phy_mode |= FIELD_PREP(PHY_MODE_SPEED_MSB, speed_msb);
	phy_mode |= FIELD_PREP(PHY_MODE_SPEED_LSB, speed_lsb);

	sw_write(priv, PHY_MODE_REG(priv->mac_idx), phy_mode);

	return 0;
}

int sw_set_duplex_mode(struct gswip_mac *priv, u32 val)
{
	u16 phy_mode;

	phy_mode = sw_read(priv, PHY_MODE_REG(priv->mac_idx));
	if (FIELD_GET(PHY_MODE_FDUP, phy_mode) != val) {
		phy_mode &= ~PHY_MODE_FDUP;
		phy_mode |= FIELD_PREP(PHY_MODE_FDUP, val);
		sw_write(priv, PHY_MODE_REG(priv->mac_idx), phy_mode);
	}

	return 0;
}

int sw_get_duplex_mode(struct gswip_mac *priv)
{
	u16 phy_mode;
	int val;

	phy_mode = sw_read(priv, PHY_STAT_REG(priv->mac_idx));
	val = FIELD_GET(PHY_STAT_FDUP, phy_mode);

	return val;
}

int sw_get_linkstatus(struct gswip_mac *priv)
{
	u16 phy_mode;
	int linkst;

	phy_mode = sw_read(priv, PHY_STAT_REG(priv->mac_idx));
	linkst = FIELD_GET(PHY_STAT_LSTAT, phy_mode);

	return linkst;
}

int sw_set_linkstatus(struct gswip_mac *priv, u8 linkst)
{
	u16 phy_mode;
	u8 val;

	phy_mode = sw_read(priv, PHY_MODE_REG(priv->mac_idx));
	val = FIELD_GET(PHY_MODE_LINKST, phy_mode);
	if (val != linkst) {
		phy_mode &= ~PHY_MODE_LINKST;
		phy_mode |= FIELD_PREP(PHY_MODE_LINKST, linkst);
		sw_write(priv, PHY_MODE_REG(priv->mac_idx), phy_mode);
	}

	return 0;
}

int sw_set_flowctrl(struct gswip_mac *priv, u8 val, u32 mode)
{
	u16 phy_mode;

	phy_mode = sw_read(priv, PHY_MODE_REG(priv->mac_idx));

	switch (mode) {
	case FCONRX:
		phy_mode &= ~PHY_MODE_FCONRX;
		phy_mode |= FIELD_PREP(PHY_MODE_FCONRX, val);
		break;

	case FCONTX:
		phy_mode &= ~PHY_MODE_FCONTX;
		phy_mode |= FIELD_PREP(PHY_MODE_FCONTX, val);
		break;
	}

	sw_write(priv, PHY_MODE_REG(priv->mac_idx), phy_mode);

	return 0;
}

u32 sw_get_speed(struct gswip_mac *priv)
{
	u16 phy_mode = 0;
	u32 speed_msb, speed_lsb, speed;

	phy_mode = sw_read(priv, PHY_STAT_REG(priv->mac_idx));
	speed_msb = FIELD_GET(PHY_STAT_SPEED_MSB, phy_mode);
	speed_lsb = FIELD_GET(PHY_STAT_SPEED_LSB, phy_mode);
	speed = (speed_msb << 2) | speed_lsb;

	return speed;
}

u32 sw_mac_get_mtu(struct gswip_mac *priv)
{
	u32 reg, val;

	reg = sw_read(priv, MAC_MTU_CFG_REG(priv->mac_idx));
	val = FIELD_PREP(MAC_MTU_CFG_MTU, reg);

	return val;
}

int sw_mac_set_mtu(struct gswip_mac *priv, u32 mtu)
{
	u32 val;

	val = sw_mac_get_mtu(priv);
	if (val != mtu)
		sw_write(priv, MAC_MTU_CFG_REG(priv->mac_idx), mtu);

	return 0;
}
