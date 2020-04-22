// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation.
 *
 * GSWIP MAC controller driver.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include "mac_common.h"

#define	MAX_MAC	9

static const char *link_status_to_str(int link)
{
	switch (link) {
	case 0:
		return "DOWN";
	case 1:
		return "UP";
	default:
		return "UNKNOWN";
	}
}

static void gswss_update_interrupt(struct gswip_mac *priv, u32 mask, u32 set)
{
	u32 val;

	val = (sw_read(priv, GSWIPSS_IER0) & ~mask) | set;
	sw_write(priv, GSWIPSS_IER0, val);
}

static void lmac_update_interrupt(struct gswip_mac *priv, u32 mask, u32 set)
{
	u32 val;

	val = (lmac_read(priv, LMAC_IER) & ~mask) | set;
	lmac_write(priv, LMAC_IER, val);
}

static void gswss_clear_interrupt_all(struct gswip_mac *priv)
{
	unsigned long xgmac_status, lmac_status;
	u32 pos;

	xgmac_status = sw_read(priv, GSWIPSS_ISR0);
	lmac_status = lmac_read(priv, LMAC_ISR);

	pos = GSWIPSS_I_XGMAC2;
	for_each_set_bit_from(pos, &xgmac_status, priv->mac_max)
		gswss_update_interrupt(priv, BIT(pos), 0);

	pos = LMAC_I_MAC2;
	for_each_set_bit_from(pos, &lmac_status, priv->mac_max)
		lmac_update_interrupt(priv, BIT(pos), 0);
}

static irqreturn_t mac_interrupt(int irq, void *data)
{
	struct gswip_mac *priv = data;

	/* clear all interrupts */
	gswss_clear_interrupt_all(priv);

	return IRQ_HANDLED;
}

/* All MAC instances have one interrupt line and need to register only once. */
static int mac_irq_init(struct gswip_mac *priv)
{
	return devm_request_irq(priv->dev, priv->sw_irq, mac_interrupt, 0,
				"gswip_mac", priv);
}

static int get_mac_index(struct device_node *node, u32 *idx)
{
	if (!strstr(node->full_name, "@"))
		return -EINVAL;

	return kstrtou32(node->full_name + strlen(node->name) + 1, 0, idx);
}

static int mac_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int linksts, duplex, speed;
	struct gswip_pdata *pdata;
	struct device_node *node;
	struct gswip_mac *priv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	node = dev->of_node;
	pdata = dev_get_platdata(dev->parent);

	/* get the platform data */
	priv->dev = &pdev->dev;
	priv->parent = dev->parent;
	priv->sw = pdata->sw;
	priv->lmac = pdata->lmac;
	priv->sw_irq = pdata->sw_irq;
	priv->ptp_clk = pdata->ptp_clk;
	priv->sw_clk = pdata->sw_clk;

	/* Initialize spin lock */
	spin_lock_init(&priv->mac_lock);
	spin_lock_init(&priv->irw_lock);
	spin_lock_init(&priv->sw_lock);

	priv->mac_max =	MAX_MAC;
	ret = get_mac_index(node, &priv->mac_idx);
	if (ret)
		return ret;

	/* check the mac index range */
	if (priv->mac_idx < 0 || priv->mac_idx > priv->mac_max) {
		dev_err(dev, "Mac index Error!!\n");
		return -EINVAL;
	}

	priv->ptp_clk_rate = clk_get_rate(priv->ptp_clk);

	ret = of_property_read_string(node, "phy-mode", &priv->phy_mode);
	if (ret)
		return ret;

	ret = of_property_read_u32(node, "speed", &priv->phy_speed);
	if (ret)
		return ret;

	/* Request IRQ on first MAC instance */
	if (!pdata->intr_flag) {
		ret = mac_irq_init(priv);
		if (ret)
			return ret;
		pdata->intr_flag = true;
	}

	dev_set_drvdata(dev, priv);
	xgmac_init_priv(priv);
	xgmac_get_hw_features(priv);
	mac_init_ops(dev);

	/* Initialize MAC */
	priv->mac_ops->init(dev);

	linksts = priv->mac_ops->get_link_sts(dev);
	duplex = priv->mac_ops->get_duplex(dev);
	speed = priv->mac_ops->get_speed(dev);

	priv->adap_ops->sw_core_enable(dev, 1);

	dev_info(dev, "Init done - Rev:%x Mac_id:%d Speed=%s Link=%s Duplex=%s\n",
		 priv->ver, priv->mac_idx, phy_speed_to_str(speed),
		 link_status_to_str(linksts), phy_duplex_to_str(duplex));

	return 0;
}

static const struct of_device_id gswip_mac_match[] = {
	{ .compatible = "gswip-mac" },
	{}
};
MODULE_DEVICE_TABLE(of, gswip_mac_match);

static struct platform_driver gswip_mac_driver = {
	.probe = mac_probe,
	.driver = {
		.name = "gswip-mac",
		.of_match_table = gswip_mac_match,
	},
};

module_platform_driver(gswip_mac_driver);
