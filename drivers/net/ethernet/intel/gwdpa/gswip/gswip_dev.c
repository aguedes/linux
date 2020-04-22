// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-2019 Intel Corporation. */

#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>

#include "gswip.h"
#include "gswip_dev.h"

#define GSWIP_SUBDEV_MAC_MAX	9
#define GSWIP_SUBDEV_CORE_MAX	1
#define GSWIP_MAC_DEV_NAME	"gswip_mac"
#define GSWIP_CORE_DEV_NAME	"gswip_core"

struct gswip_priv {
	struct device *dev;
	u32 id;
	int num_subdev_mac;
	int num_subdev_core;
	struct gswip_pdata pdata;
};

static int regmap_reg_write(void *context, unsigned int reg, unsigned int val)
{
	struct gswip_pdata *pdata = context;

	writew(val, pdata->core + reg);

	return 0;
}

static int regmap_reg_read(void *context, unsigned int reg, unsigned int *val)
{
	struct gswip_pdata *pdata = context;

	*val = readw(pdata->core + reg);

	return 0;
}

static const struct regmap_config gswip_core_regmap_config = {
	.reg_bits = 16,
	.val_bits = 16,
	.reg_stride = 4,
	.reg_write = regmap_reg_write,
	.reg_read = regmap_reg_read,
	.fast_io = true,
};

static int np_gswip_parse_dt(struct platform_device *pdev,
			     struct gswip_priv *priv)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct gswip_pdata *pdata = &priv->pdata;
	struct device_node *np;

	pdata->sw = devm_platform_ioremap_resource_byname(pdev, "switch");
	if (IS_ERR(pdata->sw))
		return PTR_ERR(pdata->sw);

	pdata->lmac = devm_platform_ioremap_resource_byname(pdev, "lmac");
	if (IS_ERR(pdata->lmac))
		return PTR_ERR(pdata->lmac);

	pdata->core = devm_platform_ioremap_resource_byname(pdev, "core");
	if (IS_ERR(pdata->core))
		return PTR_ERR(pdata->core);
	pdata->core_regmap = devm_regmap_init(dev, NULL, pdata,
					      &gswip_core_regmap_config);
	if (IS_ERR(pdata->core_regmap))
		return PTR_ERR(pdata->core_regmap);

	pdata->sw_irq = platform_get_irq_byname(pdev, "switch");
	if (pdata->sw_irq < 0)
		return -ENODEV;

	pdata->core_irq = platform_get_irq_byname(pdev, "core");
	if (pdata->core_irq < 0)
		return -ENODEV;

	pdata->ptp_clk = devm_clk_get(dev, "ptp");
	if (IS_ERR(pdata->ptp_clk))
		return PTR_ERR(pdata->ptp_clk);

	pdata->sw_clk = devm_clk_get(dev, "switch");
	if (IS_ERR(pdata->sw_clk))
		return PTR_ERR(pdata->sw_clk);

	for_each_node_by_name(node, GSWIP_MAC_DEV_NAME) {
		priv->num_subdev_mac++;
		if (priv->num_subdev_mac > GSWIP_SUBDEV_MAC_MAX) {
			dev_err(dev, "too many GSWIP mac subdevices\n");
			return -EINVAL;
		}
	}

	if (!priv->num_subdev_mac) {
		dev_err(dev, "GSWIP mac subdevice not found\n");
		return -EINVAL;
	}

	np = of_find_node_by_name(node, GSWIP_CORE_DEV_NAME);
	if (np) {
		priv->num_subdev_core++;
		of_node_put(np);
	} else {
		dev_err(dev, "GSWIP core subdevice not found\n");
		return -EINVAL;
	}

	return 0;
}

static const struct of_device_id gswip_of_match_table[] = {
	{ .compatible = "intel,lgm-gswip" },
	{}
};
MODULE_DEVICE_TABLE(of, gswip_of_match_table);

static int np_gswip_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct reset_control *rcu_reset;
	struct gswip_priv *priv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;

	ret = np_gswip_parse_dt(pdev, priv);
	if (ret) {
		dev_err(dev, "failed to parse device tree\n");
		return ret;
	}

	dev->id = priv->id;

	rcu_reset = devm_reset_control_get_optional(dev, NULL);
	if (IS_ERR(rcu_reset)) {
		dev_err(dev, "error getting reset control of gswip\n");
		return PTR_ERR(rcu_reset);
	}

	reset_control_assert(rcu_reset);
	udelay(1);
	reset_control_deassert(rcu_reset);

	dev_set_drvdata(dev, priv);

	dev->platform_data = &priv->pdata;

	ret = devm_of_platform_populate(dev);

	return ret;
}

static struct platform_driver np_gswip_driver = {
	.probe = np_gswip_probe,
	.driver = {
		.name = "np_gswip",
		.of_match_table = gswip_of_match_table,
	},
};

module_platform_driver(np_gswip_driver);

MODULE_LICENSE("GPL v2");

