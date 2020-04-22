// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019, Intel Corporation. */

/* Inter-Driver Communication */
#include "ice.h"
#include "ice_lib.h"
#include "ice_dcb_lib.h"

static struct peer_dev_id ice_peers[] = ASSIGN_PEER_INFO;

/**
 * ice_peer_state_change - manage state machine for peer
 * @peer_dev: pointer to peer's configuration
 * @new_state: the state requested to transition into
 * @locked: boolean to determine if call made with mutex held
 *
 * Any function that calls this is responsible for verifying that
 * the peer_dev_int struct is valid and capable of handling a
 * state change
 *
 * This function handles all state transitions for peer devices.
 * The state machine is as follows:
 *
 *     +<-----------------------+<-----------------------------+
 *				|<-------+<----------+	       +
 *				\/	 +	     +	       +
 *    INIT  --------------> PROBED --> OPENING	  CLOSED --> REMOVED
 *					 +           +
 *				       OPENED --> CLOSING
 *					 +	     +
 *				       PREP_RST	     +
 *					 +	     +
 *				      PREPPED	     +
 *					 +---------->+
 */
static void
ice_peer_state_change(struct ice_peer_dev_int *peer_dev, long new_state,
		      bool locked)
{
	struct device *dev = &peer_dev->peer_dev.vdev->dev;

	if (!locked)
		mutex_lock(&peer_dev->peer_dev_state_mutex);

	switch (new_state) {
	case ICE_PEER_DEV_STATE_INIT:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_REMOVED,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_INIT, peer_dev->state);
			dev_dbg(dev, "state transition from _REMOVED to _INIT\n");
		} else {
			set_bit(ICE_PEER_DEV_STATE_INIT, peer_dev->state);
			if (dev)
				dev_dbg(dev, "state set to _INIT\n");
		}
		break;
	case ICE_PEER_DEV_STATE_PROBED:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_INIT,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_PROBED, peer_dev->state);
			dev_dbg(dev, "state transition from _INIT to _PROBED\n");
		} else if (test_and_clear_bit(ICE_PEER_DEV_STATE_REMOVED,
					      peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_PROBED, peer_dev->state);
			dev_dbg(dev, "state transition from _REMOVED to _PROBED\n");
		} else if (test_and_clear_bit(ICE_PEER_DEV_STATE_OPENING,
					      peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_PROBED, peer_dev->state);
			dev_dbg(dev, "state transition from _OPENING to _PROBED\n");
		}
		break;
	case ICE_PEER_DEV_STATE_OPENING:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_PROBED,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_OPENING, peer_dev->state);
			dev_dbg(dev, "state transition from _PROBED to _OPENING\n");
		} else if (test_and_clear_bit(ICE_PEER_DEV_STATE_CLOSED,
					      peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_OPENING, peer_dev->state);
			dev_dbg(dev, "state transition from _CLOSED to _OPENING\n");
		}
		break;
	case ICE_PEER_DEV_STATE_OPENED:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_OPENING,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_OPENED, peer_dev->state);
			dev_dbg(dev, "state transition from _OPENING to _OPENED\n");
		}
		break;
	case ICE_PEER_DEV_STATE_PREP_RST:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_OPENED,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_PREP_RST, peer_dev->state);
			dev_dbg(dev, "state transition from _OPENED to _PREP_RST\n");
		}
		break;
	case ICE_PEER_DEV_STATE_PREPPED:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_PREP_RST,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_PREPPED, peer_dev->state);
			dev_dbg(dev, "state transition _PREP_RST to _PREPPED\n");
		}
		break;
	case ICE_PEER_DEV_STATE_CLOSING:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_OPENED,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_CLOSING, peer_dev->state);
			dev_dbg(dev, "state transition from _OPENED to _CLOSING\n");
		}
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_PREPPED,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_CLOSING, peer_dev->state);
			dev_dbg(dev, "state transition _PREPPED to _CLOSING\n");
		}
		/* NOTE - up to peer to handle this situation correctly */
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_PREP_RST,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_CLOSING, peer_dev->state);
			dev_warn(dev, "WARN: Peer state PREP_RST to _CLOSING\n");
		}
		break;
	case ICE_PEER_DEV_STATE_CLOSED:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_CLOSING,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_CLOSED, peer_dev->state);
			dev_dbg(dev, "state transition from _CLOSING to _CLOSED\n");
		}
		break;
	case ICE_PEER_DEV_STATE_REMOVED:
		if (test_and_clear_bit(ICE_PEER_DEV_STATE_OPENED,
				       peer_dev->state) ||
		    test_and_clear_bit(ICE_PEER_DEV_STATE_CLOSED,
				       peer_dev->state)) {
			set_bit(ICE_PEER_DEV_STATE_REMOVED, peer_dev->state);
			dev_dbg(dev, "state from _OPENED/_CLOSED to _REMOVED\n");
			/* Clear registration for events when peer removed */
			bitmap_zero(peer_dev->events, ICE_PEER_DEV_STATE_NBITS);
		}
		break;
	default:
		break;
	}

	if (!locked)
		mutex_unlock(&peer_dev->peer_dev_state_mutex);
}

/**
 * ice_peer_close - close a peer device
 * @peer_dev_int: device to close
 * @data: pointer to opaque data
 *
 * This function will also set the state bit for the peer to CLOSED. This
 * function is meant to be called from a ice_for_each_peer().
 */
int ice_peer_close(struct ice_peer_dev_int *peer_dev_int, void *data)
{
	enum iidc_close_reason reason = *(enum iidc_close_reason *)(data);
	struct iidc_peer_dev *peer_dev;
	struct ice_pf *pf;
	int i;

	peer_dev = &peer_dev_int->peer_dev;
	/* return 0 so ice_for_each_peer will continue closing other peers */
	if (!ice_validate_peer_dev(peer_dev))
		return 0;
	pf = pci_get_drvdata(peer_dev->pdev);

	if (test_bit(__ICE_DOWN, pf->state) ||
	    test_bit(__ICE_SUSPENDED, pf->state) ||
	    test_bit(__ICE_NEEDS_RESTART, pf->state))
		return 0;

	mutex_lock(&peer_dev_int->peer_dev_state_mutex);

	/* no peer driver, already closed, closing or opening nothing to do */
	if (test_bit(ICE_PEER_DEV_STATE_CLOSED, peer_dev_int->state) ||
	    test_bit(ICE_PEER_DEV_STATE_CLOSING, peer_dev_int->state) ||
	    test_bit(ICE_PEER_DEV_STATE_OPENING, peer_dev_int->state) ||
	    test_bit(ICE_PEER_DEV_STATE_REMOVED, peer_dev_int->state))
		goto peer_close_out;

	/* Set the peer state to CLOSING */
	ice_peer_state_change(peer_dev_int, ICE_PEER_DEV_STATE_CLOSING, true);

	for (i = 0; i < IIDC_EVENT_NBITS; i++)
		bitmap_zero(peer_dev_int->current_events[i].type,
			    IIDC_EVENT_NBITS);

	if (peer_dev->peer_ops && peer_dev->peer_ops->close)
		peer_dev->peer_ops->close(peer_dev, reason);

	/* Set the peer state to CLOSED */
	ice_peer_state_change(peer_dev_int, ICE_PEER_DEV_STATE_CLOSED, true);

peer_close_out:
	mutex_unlock(&peer_dev_int->peer_dev_state_mutex);

	return 0;
}

/**
 * ice_peer_update_vsi - update the pf_vsi info in peer_dev struct
 * @peer_dev_int: pointer to peer dev internal struct
 * @data: opaque pointer - VSI to be updated
 */
int ice_peer_update_vsi(struct ice_peer_dev_int *peer_dev_int, void *data)
{
	struct ice_vsi *vsi = (struct ice_vsi *)data;
	struct iidc_peer_dev *peer_dev;

	peer_dev = &peer_dev_int->peer_dev;
	if (!peer_dev)
		return 0;

	peer_dev->pf_vsi_num = vsi->vsi_num;
	return 0;
}

/**
 * ice_for_each_peer - iterate across and call function for each peer dev
 * @pf: pointer to private board struct
 * @data: data to pass to function on each call
 * @fn: pointer to function to call for each peer
 */
int
ice_for_each_peer(struct ice_pf *pf, void *data,
		  int (*fn)(struct ice_peer_dev_int *, void *))
{
	unsigned int i;

	if (!pf->peers)
		return 0;

	for (i = 0; i < ARRAY_SIZE(ice_peers); i++) {
		struct ice_peer_dev_int *peer_dev_int;

		peer_dev_int = pf->peers[i];
		if (peer_dev_int) {
			int ret = fn(peer_dev_int, data);

			if (ret)
				return ret;
		}
	}

	return 0;
}

/**
 * ice_finish_init_peer_device - complete peer device initialization
 * @peer_dev_int: ptr to peer device internal struct
 * @data: ptr to opaque data
 *
 * This function completes remaining initialization of peer_devices
 */
int
ice_finish_init_peer_device(struct ice_peer_dev_int *peer_dev_int,
			    void __always_unused *data)
{
	struct iidc_peer_dev *peer_dev;
	struct iidc_peer_drv *peer_drv;
	struct device *dev;
	struct ice_pf *pf;
	int ret = 0;

	peer_dev = &peer_dev_int->peer_dev;
	/* peer_dev will not always be populated at the time of this check */
	if (!ice_validate_peer_dev(peer_dev))
		return ret;

	peer_drv = peer_dev->peer_drv;
	pf = pci_get_drvdata(peer_dev->pdev);
	dev = ice_pf_to_dev(pf);
	/* There will be several assessments of the peer_dev's state in this
	 * chunk of logic.  We need to hold the peer_dev_int's state mutex
	 * for the entire part so that the flow progresses without another
	 * context changing things mid-flow
	 */
	mutex_lock(&peer_dev_int->peer_dev_state_mutex);

	if (!peer_dev->peer_ops) {
		dev_err(dev, "peer_ops not defined on peer dev\n");
		goto init_unlock;
	}

	if (!peer_dev->peer_ops->open) {
		dev_err(dev, "peer_ops:open not defined on peer dev\n");
		goto init_unlock;
	}

	if (!peer_dev->peer_ops->close) {
		dev_err(dev, "peer_ops:close not defined on peer dev\n");
		goto init_unlock;
	}

	/* Peer driver expected to set driver_id during registration */
	if (!peer_drv->driver_id) {
		dev_err(dev, "Peer driver did not set driver_id\n");
		goto init_unlock;
	}

	if ((test_bit(ICE_PEER_DEV_STATE_CLOSED, peer_dev_int->state) ||
	     test_bit(ICE_PEER_DEV_STATE_PROBED, peer_dev_int->state)) &&
	    ice_pf_state_is_nominal(pf)) {
		/* If the RTNL is locked, we defer opening the peer
		 * until the next time this function is called by the
		 * service task.
		 */
		if (rtnl_is_locked())
			goto init_unlock;
		ice_peer_state_change(peer_dev_int, ICE_PEER_DEV_STATE_OPENING,
				      true);
		ret = peer_dev->peer_ops->open(peer_dev);
		if (ret) {
			dev_err(dev, "Peer %d failed to open\n",
				peer_dev->peer_dev_id);
			ice_peer_state_change(peer_dev_int,
					      ICE_PEER_DEV_STATE_PROBED, true);
			goto init_unlock;
		}

		ice_peer_state_change(peer_dev_int, ICE_PEER_DEV_STATE_OPENED,
				      true);
	}

init_unlock:
	mutex_unlock(&peer_dev_int->peer_dev_state_mutex);

	return ret;
}

/**
 * ice_unreg_peer_device - unregister specified device
 * @peer_dev_int: ptr to peer device internal
 * @data: ptr to opaque data
 *
 * This function invokes device unregistration, removes ID associated with
 * the specified device.
 */
int
ice_unreg_peer_device(struct ice_peer_dev_int *peer_dev_int,
		      void __always_unused *data)
{
	struct ice_peer_drv_int *peer_drv_int;

	if (!peer_dev_int)
		return 0;

	virtbus_unregister_device(peer_dev_int->peer_dev.vdev);

	peer_drv_int = peer_dev_int->peer_drv_int;

	if (peer_dev_int->ice_peer_wq) {
		if (peer_dev_int->peer_prep_task.func)
			cancel_work_sync(&peer_dev_int->peer_prep_task);

		if (peer_dev_int->peer_close_task.func)
			cancel_work_sync(&peer_dev_int->peer_close_task);
		destroy_workqueue(peer_dev_int->ice_peer_wq);
	}

	kfree(peer_drv_int);

	kfree(peer_dev_int);

	return 0;
}

/**
 * ice_unroll_peer - destroy peers and peer_wq in case of error
 * @peer_dev_int: ptr to peer device internal struct
 * @data: ptr to opaque data
 *
 * This function releases resources in the event of a failure in creating
 * peer devices or their individual work_queues. Meant to be called from
 * a ice_for_each_peer invocation
 */
int
ice_unroll_peer(struct ice_peer_dev_int *peer_dev_int,
		void __always_unused *data)
{
	if (peer_dev_int->ice_peer_wq)
		destroy_workqueue(peer_dev_int->ice_peer_wq);
	kfree(peer_dev_int);

	return 0;
}

/**
 * ice_find_vsi - Find the VSI from VSI ID
 * @pf: The PF pointer to search in
 * @vsi_num: The VSI ID to search for
 */
static struct ice_vsi *ice_find_vsi(struct ice_pf *pf, u16 vsi_num)
{
	int i;

	ice_for_each_vsi(pf, i)
		if (pf->vsi[i] && pf->vsi[i]->vsi_num == vsi_num)
			return  pf->vsi[i];
	return NULL;
}

/**
 * ice_peer_alloc_rdma_qsets - Allocate Leaf Nodes for RDMA Qset
 * @peer_dev: peer that is requesting the Leaf Nodes
 * @res: Resources to be allocated
 * @partial_acceptable: If partial allocation is acceptable to the peer
 *
 * This function allocates Leaf Nodes for given RDMA Qset resources
 * for the peer device.
 */
static int
ice_peer_alloc_rdma_qsets(struct iidc_peer_dev *peer_dev, struct iidc_res *res,
			  int __always_unused partial_acceptable)
{
	u16 max_rdmaqs[ICE_MAX_TRAFFIC_CLASS];
	enum ice_status status;
	struct ice_vsi *vsi;
	struct device *dev;
	struct ice_pf *pf;
	int i, ret = 0;
	u32 *qset_teid;
	u16 *qs_handle;

	if (!ice_validate_peer_dev(peer_dev) || !res)
		return -EINVAL;

	pf = pci_get_drvdata(peer_dev->pdev);
	dev = ice_pf_to_dev(pf);

	if (res->cnt_req > ICE_MAX_TXQ_PER_TXQG)
		return -EINVAL;

	qset_teid = kcalloc(res->cnt_req, sizeof(*qset_teid), GFP_KERNEL);
	if (!qset_teid)
		return -ENOMEM;

	qs_handle = kcalloc(res->cnt_req, sizeof(*qs_handle), GFP_KERNEL);
	if (!qs_handle) {
		kfree(qset_teid);
		return -ENOMEM;
	}

	ice_for_each_traffic_class(i)
		max_rdmaqs[i] = 0;

	for (i = 0; i < res->cnt_req; i++) {
		struct iidc_rdma_qset_params *qset;

		qset = &res->res[i].res.qsets;
		if (qset->vsi_id != peer_dev->pf_vsi_num) {
			dev_err(dev, "RDMA QSet invalid VSI requested\n");
			ret = -EINVAL;
			goto out;
		}
		max_rdmaqs[qset->tc]++;
		qs_handle[i] = qset->qs_handle;
	}

	vsi = ice_find_vsi(pf, peer_dev->pf_vsi_num);
	if (!vsi) {
		dev_err(dev, "RDMA QSet invalid VSI\n");
		ret = -EINVAL;
		goto out;
	}

	status = ice_cfg_vsi_rdma(vsi->port_info, vsi->idx, vsi->tc_cfg.ena_tc,
				  max_rdmaqs);
	if (status) {
		dev_err(dev, "Failed VSI RDMA qset config\n");
		ret = -EINVAL;
		goto out;
	}

	for (i = 0; i < res->cnt_req; i++) {
		struct iidc_rdma_qset_params *qset;

		qset = &res->res[i].res.qsets;
		status = ice_ena_vsi_rdma_qset(vsi->port_info, vsi->idx,
					       qset->tc, &qs_handle[i], 1,
					       &qset_teid[i]);
		if (status) {
			dev_err(dev, "Failed VSI RDMA qset enable\n");
			ret = -EINVAL;
			goto out;
		}
		vsi->qset_handle[qset->tc] = qset->qs_handle;
		qset->teid = qset_teid[i];
	}

out:
	kfree(qset_teid);
	kfree(qs_handle);
	return ret;
}

/**
 * ice_peer_free_rdma_qsets - Free leaf nodes for RDMA Qset
 * @peer_dev: peer that requested qsets to be freed
 * @res: Resource to be freed
 */
static int
ice_peer_free_rdma_qsets(struct iidc_peer_dev *peer_dev, struct iidc_res *res)
{
	enum ice_status status;
	int count, i, ret = 0;
	struct ice_vsi *vsi;
	struct device *dev;
	struct ice_pf *pf;
	u16 vsi_id;
	u32 *teid;
	u16 *q_id;

	if (!ice_validate_peer_dev(peer_dev) || !res)
		return -EINVAL;

	pf = pci_get_drvdata(peer_dev->pdev);
	dev = ice_pf_to_dev(pf);

	count = res->res_allocated;
	if (count > ICE_MAX_TXQ_PER_TXQG)
		return -EINVAL;

	teid = kcalloc(count, sizeof(*teid), GFP_KERNEL);
	if (!teid)
		return -ENOMEM;

	q_id = kcalloc(count, sizeof(*q_id), GFP_KERNEL);
	if (!q_id) {
		kfree(teid);
		return -ENOMEM;
	}

	vsi_id = res->res[0].res.qsets.vsi_id;
	vsi = ice_find_vsi(pf, vsi_id);
	if (!vsi) {
		dev_err(dev, "RDMA Invalid VSI\n");
		ret = -EINVAL;
		goto rdma_free_out;
	}

	for (i = 0; i < count; i++) {
		struct iidc_rdma_qset_params *qset;

		qset = &res->res[i].res.qsets;
		if (qset->vsi_id != vsi_id) {
			dev_err(dev, "RDMA Invalid VSI ID\n");
			ret = -EINVAL;
			goto rdma_free_out;
		}
		q_id[i] = qset->qs_handle;
		teid[i] = qset->teid;

		vsi->qset_handle[qset->tc] = 0;
	}

	status = ice_dis_vsi_rdma_qset(vsi->port_info, count, teid, q_id);
	if (status)
		ret = -EINVAL;

rdma_free_out:
	kfree(teid);
	kfree(q_id);

	return ret;
}

/**
 * ice_peer_alloc_res - Allocate requested resources for peer device
 * @peer_dev: peer that is requesting resources
 * @res: Resources to be allocated
 * @partial_acceptable: If partial allocation is acceptable to the peer
 *
 * This function allocates requested resources for the peer device.
 */
static int
ice_peer_alloc_res(struct iidc_peer_dev *peer_dev, struct iidc_res *res,
		   int partial_acceptable)
{
	struct ice_pf *pf;
	int ret;

	if (!ice_validate_peer_dev(peer_dev) || !res)
		return -EINVAL;

	pf = pci_get_drvdata(peer_dev->pdev);
	if (!ice_pf_state_is_nominal(pf))
		return -EBUSY;

	switch (res->res_type) {
	case IIDC_RDMA_QSETS_TXSCHED:
		ret = ice_peer_alloc_rdma_qsets(peer_dev, res,
						partial_acceptable);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * ice_peer_free_res - Free given resources
 * @peer_dev: peer that is requesting freeing of resources
 * @res: Resources to be freed
 *
 * Free/Release resources allocated to given peer device.
 */
static int
ice_peer_free_res(struct iidc_peer_dev *peer_dev, struct iidc_res *res)
{
	int ret;

	if (!ice_validate_peer_dev(peer_dev) || !res)
		return -EINVAL;

	switch (res->res_type) {
	case IIDC_RDMA_QSETS_TXSCHED:
		ret = ice_peer_free_rdma_qsets(peer_dev, res);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * ice_peer_unregister - request to unregister peer
 * @peer_dev: peer device
 *
 * This function triggers close/remove on peer_dev allowing peer
 * to unregister.
 */
static int ice_peer_unregister(struct iidc_peer_dev *peer_dev)
{
	enum iidc_close_reason reason = IIDC_REASON_PEER_DEV_UNINIT;
	struct ice_peer_dev_int *peer_dev_int;
	struct ice_pf *pf;
	int ret;

	if (!ice_validate_peer_dev(peer_dev))
		return -EINVAL;

	pf = pci_get_drvdata(peer_dev->pdev);
	if (ice_is_reset_in_progress(pf->state))
		return -EBUSY;

	peer_dev_int = peer_to_ice_dev_int(peer_dev);

	ret = ice_peer_close(peer_dev_int, &reason);
	if (ret)
		return ret;

	peer_dev->peer_ops = NULL;

	ice_peer_state_change(peer_dev_int, ICE_PEER_DEV_STATE_REMOVED, false);

	return 0;
}

/**
 * ice_peer_register - Called by peer to open communication with LAN
 * @peer_dev: ptr to peer device
 *
 * registering peer is expected to populate the ice_peerdrv->name field
 * before calling this function.
 */
static int ice_peer_register(struct iidc_peer_dev *peer_dev)
{
	struct ice_peer_drv_int *peer_drv_int;
	struct ice_peer_dev_int *peer_dev_int;
	struct iidc_peer_drv *peer_drv;

	if (!peer_dev) {
		pr_err("Failed to reg peer dev: peer_dev ptr NULL\n");
		return -EINVAL;
	}

	if (!peer_dev->pdev) {
		pr_err("Failed to reg peer dev: peer dev pdev NULL\n");
		return -EINVAL;
	}

	if (!peer_dev->peer_ops || !peer_dev->ops) {
		pr_err("Failed to reg peer dev: peer dev peer_ops/ops NULL\n");
		return -EINVAL;
	}

	peer_drv = peer_dev->peer_drv;
	if (!peer_drv) {
		pr_err("Failed to reg peer dev: peer drv NULL\n");
		return -EINVAL;
	}

	peer_dev_int = peer_to_ice_dev_int(peer_dev);
	peer_drv_int = peer_dev_int->peer_drv_int;
	if (!peer_drv_int) {
		pr_err("Failed to match peer_drv_int to peer_dev\n");
		return -EINVAL;
	}

	peer_drv_int->peer_drv = peer_drv;

	ice_peer_state_change(peer_dev_int, ICE_PEER_DEV_STATE_PROBED, false);

	return 0;
}

/**
 * ice_peer_update_vsi_filter - update main VSI filters for RDMA
 * @peer_dev: pointer to RDMA peer device
 * @filter: selection of filters to enable or disable
 * @enable: bool whether to enable or disable filters
 */
static int
ice_peer_update_vsi_filter(struct iidc_peer_dev *peer_dev,
			   enum iidc_rdma_filter __always_unused filter,
			   bool enable)
{
	struct ice_vsi *vsi;
	struct ice_pf *pf;
	int ret;

	if (!ice_validate_peer_dev(peer_dev))
		return -EINVAL;

	pf = pci_get_drvdata(peer_dev->pdev);

	vsi = ice_get_main_vsi(pf);
	if (!vsi)
		return -EINVAL;

	ret = ice_cfg_iwarp_fltr(&pf->hw, vsi->idx, enable);

	if (ret) {
		dev_err(ice_pf_to_dev(pf), "Failed to  %sable iWARP filtering\n",
			enable ? "en" : "dis");
	} else {
		if (enable)
			vsi->info.q_opt_flags |= ICE_AQ_VSI_Q_OPT_PE_FLTR_EN;
		else
			vsi->info.q_opt_flags &= ~ICE_AQ_VSI_Q_OPT_PE_FLTR_EN;
	}

	return ret;
}

/* Initialize the ice_ops struct, which is used in 'ice_init_peer_devices' */
static const struct iidc_ops ops = {
	.alloc_res			= ice_peer_alloc_res,
	.free_res			= ice_peer_free_res,
	.peer_register			= ice_peer_register,
	.peer_unregister		= ice_peer_unregister,
	.update_vsi_filter		= ice_peer_update_vsi_filter,
};

/**
 * ice_reserve_peer_qvector - Reserve vector resources for peer drivers
 * @pf: board private structure to initialize
 */
static int ice_reserve_peer_qvector(struct ice_pf *pf)
{
	if (test_bit(ICE_FLAG_IWARP_ENA, pf->flags)) {
		int index;

		index = ice_get_res(pf, pf->irq_tracker, pf->num_rdma_msix,
				    ICE_RES_RDMA_VEC_ID);
		if (index < 0)
			return index;
		pf->num_avail_sw_msix -= pf->num_rdma_msix;
		pf->rdma_base_vector = index;
	}
	return 0;
}

/**
 * ice_peer_vdev_release - function to map to virtbus_devices release callback
 * @vdev: pointer to virtbus_device to free
 */
static void ice_peer_vdev_release(struct virtbus_device *vdev)
{
	struct iidc_virtbus_object *vbo;

	vbo = container_of(vdev, struct iidc_virtbus_object, vdev);
	kfree(vbo);
}

/**
 * ice_init_peer_devices - initializes peer devices
 * @pf: ptr to ice_pf
 *
 * This function initializes peer devices on the virtual bus.
 */
int ice_init_peer_devices(struct ice_pf *pf)
{
	struct ice_vsi *vsi = pf->vsi[0];
	struct pci_dev *pdev = pf->pdev;
	struct device *dev = &pdev->dev;
	int status = 0;
	unsigned int i;

	/* Reserve vector resources */
	status = ice_reserve_peer_qvector(pf);
	if (status < 0) {
		dev_err(dev, "failed to reserve vectors for peer drivers\n");
		return status;
	}
	for (i = 0; i < ARRAY_SIZE(ice_peers); i++) {
		struct ice_peer_dev_int *peer_dev_int;
		struct ice_peer_drv_int *peer_drv_int;
		struct iidc_qos_params *qos_info;
		struct iidc_virtbus_object *vbo;
		struct msix_entry *entry = NULL;
		struct iidc_peer_dev *peer_dev;
		struct virtbus_device *vdev;
		int j;

		/* structure layout needed for container_of's looks like:
		 * ice_peer_dev_int (internal only ice peer superstruct)
		 * |--> iidc_peer_dev
		 * |--> *ice_peer_drv_int
		 *
		 * iidc_virtbus_object (container_of parent for vdev)
		 * |--> virtbus_device
		 * |--> *iidc_peer_dev (pointer from internal struct)
		 *
		 * ice_peer_drv_int (internal only peer_drv struct)
		 */
		peer_dev_int = kzalloc(sizeof(*peer_dev_int), GFP_KERNEL);
		if (!peer_dev_int)
			return -ENOMEM;

		vbo = kzalloc(sizeof(*vbo), GFP_KERNEL);
		if (!vbo) {
			kfree(peer_dev_int);
			return -ENOMEM;
		}

		peer_drv_int = kzalloc(sizeof(*peer_drv_int), GFP_KERNEL);
		if (!peer_drv_int) {
			kfree(peer_dev_int);
			kfree(vbo);
			return -ENOMEM;
		}

		pf->peers[i] = peer_dev_int;
		vbo->peer_dev = &peer_dev_int->peer_dev;
		peer_dev_int->peer_drv_int = peer_drv_int;
		peer_dev_int->peer_dev.vdev = &vbo->vdev;

		/* Initialize driver values */
		for (j = 0; j < IIDC_EVENT_NBITS; j++)
			bitmap_zero(peer_drv_int->current_events[j].type,
				    IIDC_EVENT_NBITS);

		mutex_init(&peer_dev_int->peer_dev_state_mutex);

		peer_dev = &peer_dev_int->peer_dev;
		peer_dev->peer_ops = NULL;
		peer_dev->hw_addr = (u8 __iomem *)pf->hw.hw_addr;
		peer_dev->peer_dev_id = ice_peers[i].id;
		peer_dev->pf_vsi_num = vsi->vsi_num;
		peer_dev->netdev = vsi->netdev;

		peer_dev_int->ice_peer_wq =
			alloc_ordered_workqueue("ice_peer_wq_%d", WQ_UNBOUND,
						i);
		if (!peer_dev_int->ice_peer_wq) {
			kfree(peer_dev_int);
			kfree(peer_drv_int);
			kfree(vbo);
			return -ENOMEM;
		}

		peer_dev->pdev = pdev;
		qos_info = &peer_dev->initial_qos_info;

		/* setup qos_info fields with defaults */
		qos_info->num_apps = 0;
		qos_info->num_tc = 1;

		for (j = 0; j < IIDC_MAX_USER_PRIORITY; j++)
			qos_info->up2tc[j] = 0;

		qos_info->tc_info[0].rel_bw = 100;
		for (j = 1; j < IEEE_8021QAZ_MAX_TCS; j++)
			qos_info->tc_info[j].rel_bw = 0;

		/* for DCB, override the qos_info defaults. */
		ice_setup_dcb_qos_info(pf, qos_info);
		/* Initialize ice_ops */
		peer_dev->ops = &ops;

		/* make sure peer specific resources such as msix_count and
		 * msix_entries are initialized
		 */
		switch (ice_peers[i].id) {
		case IIDC_PEER_RDMA_ID:
			if (test_bit(ICE_FLAG_IWARP_ENA, pf->flags)) {
				peer_dev->msix_count = pf->num_rdma_msix;
				entry = &pf->msix_entries[pf->rdma_base_vector];
			}
			break;
		default:
			break;
		}

		peer_dev->msix_entries = entry;
		ice_peer_state_change(peer_dev_int, ICE_PEER_DEV_STATE_INIT,
				      false);

		vdev = &vbo->vdev;
		vdev->name = ice_peers[i].name;
		vdev->release = ice_peer_vdev_release;
		vdev->dev.parent = &pdev->dev;

		status = virtbus_register_device(vdev);
		if (status) {
			kfree(peer_dev_int);
			kfree(peer_drv_int);
			vdev = NULL;
			return status;
		}
	}

	return status;
}
