// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020, Intel Corporation. */

#include <linux/bpf_trace.h>

#include "igc.h"
#include "igc_xdp.h"

int igc_xdp_set_prog(struct igc_adapter *adapter, struct bpf_prog *prog,
		     struct netlink_ext_ack *extack)
{
	struct net_device *dev = adapter->netdev;
	bool if_running = netif_running(dev);
	struct bpf_prog *old_prog;

	if (dev->mtu > ETH_DATA_LEN) {
		/* For now, the driver doesn't support XDP functionality with
		 * jumbo frames so we return error.
		 */
		NL_SET_ERR_MSG_MOD(extack, "Jumbo frames not supported");
		return -EOPNOTSUPP;
	}

	if (if_running)
		igc_close(dev);

	old_prog = xchg(&adapter->xdp_prog, prog);
	if (old_prog)
		bpf_prog_put(old_prog);

	if (if_running)
		igc_open(dev);

	netdev_dbg(dev, "XDP program set successfully");
	return 0;
}

struct sk_buff *igc_xdp_run_prog(struct igc_adapter *adapter,
				 struct xdp_buff *xdp)
{
	struct bpf_prog *prog;
	int res;
	u32 act;

	rcu_read_lock();

	prog = READ_ONCE(adapter->xdp_prog);
	if (!prog)
		goto unlock;

	act = bpf_prog_run_xdp(prog, xdp);
	switch (act) {
	case XDP_PASS:
		res = IGC_XDP_PASS;
		break;
	default:
		bpf_warn_invalid_xdp_action(act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(adapter->netdev, prog, act);
		fallthrough;
	case XDP_DROP:
		res = IGC_XDP_CONSUMED;
		break;
	}

unlock:
	rcu_read_unlock();
	return ERR_PTR(-res);
}

bool igc_xdp_is_enabled(struct igc_adapter *adapter)
{
	return !!adapter->xdp_prog;
}
