/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020, Intel Corporation. */

#ifndef _IECM_TYPE_H_
#define _IECM_TYPE_H_

#include <linux/if_ether.h>
#include "iecm_osdep.h"
#include "iecm_alloc.h"
#include "iecm_controlq.h"

#define MAKEMASK(m, s)	((m) << (s))

/* Bus parameters */
struct iecm_bus_info {
	u16 func;
	u16 device;
	u16 bus_id;
};

/* Define the APF hardware struct to replace other control structs as needed
 * Align to ctlq_hw_info
 */
struct iecm_hw {
	u8 *hw_addr;
	u64 hw_addr_len;
	void *back;

	/* control queue - send and receive */
	struct iecm_ctlq_info *asq;
	struct iecm_ctlq_info *arq;

	/* subsystem structs */
	struct iecm_bus_info bus;

	/* pci info */
	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	u8 revision_id;
	bool adapter_stopped;

	struct list_head cq_list_head;
};

#endif /* _IECM_TYPE_H_ */
