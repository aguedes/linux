/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020, Intel Corporation. */

#ifndef _IGC_XDP_H_
#define _IGC_XDP_H_

#define IGC_XDP_PASS		0
#define IGC_XDP_CONSUMED	BIT(0)
#define IGC_XDP_TX		BIT(1)
#define IGC_XDP_REDIRECT	BIT(2)

int igc_xdp_set_prog(struct igc_adapter *adapter, struct bpf_prog *prog,
		     struct netlink_ext_ack *extack);

struct sk_buff *igc_xdp_run_prog(struct igc_adapter *adapter,
				 struct xdp_buff *xdp);

bool igc_xdp_is_enabled(struct igc_adapter *adapter);

int igc_xdp_register_rxq_info(struct igc_ring *ring);
void igc_xdp_unregister_rxq_info(struct igc_ring *ring);

struct igc_ring *igc_xdp_get_tx_ring(struct igc_adapter *adapter, int cpu);

#endif /* _IGC_XDP_H_ */
