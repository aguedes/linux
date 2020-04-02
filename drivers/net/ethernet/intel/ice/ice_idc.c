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
