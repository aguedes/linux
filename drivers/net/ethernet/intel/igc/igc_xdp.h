/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020, Intel Corporation. */

#ifndef _IGC_XDP_H_
#define _IGC_XDP_H_

#define IGC_XDP_PASS		0
#define IGC_XDP_CONSUMED	BIT(0)

int igc_xdp_set_prog(struct igc_adapter *adapter, struct bpf_prog *prog,
		     struct netlink_ext_ack *extack);

struct sk_buff *igc_xdp_run_prog(struct igc_adapter *adapter,
				 struct xdp_buff *xdp);

bool igc_xdp_is_enabled(struct igc_adapter *adapter);

#endif /* _IGC_XDP_H_ */
