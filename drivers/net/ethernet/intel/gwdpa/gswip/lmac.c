// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation. */

#include <linux/bitfield.h>
#include <linux/types.h>

#include "mac_common.h"

int lmac_set_duplex_mode(struct gswip_mac *priv, u32 val)
{
	u32 mac_ctrl0 = lmac_read(priv, MAC_CTRL0_REG(priv->mac_idx));

	if (FIELD_GET(MAC_CTRL0_FDUP, mac_ctrl0) != val) {
		mac_ctrl0 &= ~MAC_CTRL0_FDUP;
		mac_ctrl0 |= FIELD_PREP(MAC_CTRL0_FDUP, val);
		lmac_write(priv, MAC_CTRL0_REG(priv->mac_idx), mac_ctrl0);
	}

	return 0;
}

int lmac_set_flowcon_mode(struct gswip_mac *priv, u32 val)
{
	u32 mac_ctrl0 = lmac_read(priv, MAC_CTRL0_REG(priv->mac_idx));

	if (FIELD_GET(MAC_CTRL0_FCON, mac_ctrl0) != val) {
		mac_ctrl0 &= ~MAC_CTRL0_FCON;
		mac_ctrl0 |= FIELD_PREP(MAC_CTRL0_FCON, val);
		lmac_write(priv, MAC_CTRL0_REG(priv->mac_idx), mac_ctrl0);
	}

	return 0;
}

int lmac_set_intf_mode(struct gswip_mac *priv, u32 val)
{
	u32 mac_ctrl0 = lmac_read(priv, MAC_CTRL0_REG(priv->mac_idx));

	if (FIELD_GET(MAC_CTRL0_GMII, mac_ctrl0) != val) {
		mac_ctrl0 &= ~MAC_CTRL0_GMII;
		mac_ctrl0 |= FIELD_PREP(MAC_CTRL0_GMII, val);
		lmac_write(priv, MAC_CTRL0_REG(priv->mac_idx), mac_ctrl0);
	}

	return 0;
}
