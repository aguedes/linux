===============================
Virtual Bus Devices and Drivers
===============================

See <linux/virtual_bus.h> for the models for virtbus_device and virtbus_driver.

This bus is meant to be a minimalist software-based bus used for
connecting devices (that may not physically exist) to be able to
communicate with each other.


Memory Allocation Lifespan and Model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The memory for a virtbus_device or virtbus_driver needs to be
allocated before registering them on the virtual bus.

The memory for the virtual_device is expected to remain viable until
the device's mandatory .release() callback is invoked when the device
is unregistered by calling virtbus_unregister_device().

Memory associated with a virtbus_driver is expected to remain viable
until the driver's .remove() or .shutdown() callbacks are invoked
during module insertion or removal.

Device Enumeration
~~~~~~~~~~~~~~~~~~

The virtbus device is enumerated when it is attached to the bus. The
device is assigned a unique ID that will be appended to its name
making it unique.  If two virtbus_devices both named "foo" are
registered onto the bus, they will have a sub-device names of "foo.x"
and "foo.y" where x and y are unique integers.

Common Usage and Structure Design
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The virtbus_device and virtbus_driver need to have a common header
file.

In the common header file outside of the virtual_bus infrastructure,
define struct virtbus_object:

.. code-block:: c

        struct virtbus_object {
                virtbus_device vdev;
                struct my_private_struct *my_stuff;
        }

When the virtbus_device vdev is passed to the virtbus_driver's probe
callback, it can then get access to the struct my_stuff.

An example of the driver encapsulation:

.. code-block:: c

	struct custom_driver {
		struct virtbus_driver virtbus_drv;
		const struct custom_driver_ops ops;
	}

An example of this usage would be :

.. code-block:: c

	struct custom_driver custom_drv = {
		.virtbus_drv = {
			.driver = {
				.name = "sof-ipc-test-virtbus-drv",
			},
			.id_table = custom_virtbus_id_table,
			.probe = custom_probe,
			.remove = custom_remove,
			.shutdown = custom_shutdown,
		},
		.ops = custom_ops,
	};

Mandatory Elements
~~~~~~~~~~~~~~~~~~

virtbus_device:

- .release() callback must not be NULL and is expected to perform memory cleanup.
- .match_name must be populated to be able to match with a driver

virtbus_driver:

- .probe() callback must not be NULL
- .remove() callback must not be NULL
- .shutdown() callback must not be NULL
- .id_table must not be NULL, used to perform matching

