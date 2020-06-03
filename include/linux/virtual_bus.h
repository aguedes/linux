/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * virtual_bus.h - lightweight software bus
 *
 * Copyright (c) 2019-2020 Intel Corporation
 *
 * Please see Documentation/driver-api/virtual_bus.rst for more information
 */

#ifndef _VIRTUAL_BUS_H_
#define _VIRTUAL_BUS_H_

#include <linux/device.h>

struct virtbus_device {
	struct device dev;
	const char *match_name;
	void (*release)(struct virtbus_device *);
	u32 id;
};

struct virtbus_driver {
	int (*probe)(struct virtbus_device *);
	int (*remove)(struct virtbus_device *);
	void (*shutdown)(struct virtbus_device *);
	int (*suspend)(struct virtbus_device *, pm_message_t);
	int (*resume)(struct virtbus_device *);
	struct device_driver driver;
	const struct virtbus_dev_id *id_table;
};

static inline
struct virtbus_device *to_virtbus_dev(struct device *dev)
{
	return container_of(dev, struct virtbus_device, dev);
}

static inline
struct virtbus_driver *to_virtbus_drv(struct device_driver *drv)
{
	return container_of(drv, struct virtbus_driver, driver);
}

int virtbus_register_device(struct virtbus_device *vdev);

int
__virtbus_register_driver(struct virtbus_driver *vdrv, struct module *owner);

#define virtbus_register_driver(vdrv) \
	__virtbus_register_driver(vdrv, THIS_MODULE)

static inline void virtbus_unregister_device(struct virtbus_device *vdev)
{
	device_unregister(&vdev->dev);
}

static inline void virtbus_unregister_driver(struct virtbus_driver *vdrv)
{
	driver_unregister(&vdrv->driver);
}

#endif /* _VIRTUAL_BUS_H_ */
