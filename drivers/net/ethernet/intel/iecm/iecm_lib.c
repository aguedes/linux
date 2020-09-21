// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Intel Corporation */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/net/intel/iecm.h>

static const struct net_device_ops iecm_netdev_ops_splitq;
static const struct net_device_ops iecm_netdev_ops_singleq;

/**
 * iecm_mb_intr_rel_irq - Free the IRQ association with the OS
 * @adapter: adapter structure
 */
static void iecm_mb_intr_rel_irq(struct iecm_adapter *adapter)
{
	int irq_num;

	irq_num = adapter->msix_entries[0].vector;
	free_irq(irq_num, adapter);
}

/**
 * iecm_intr_rel - Release interrupt capabilities and free memory
 * @adapter: adapter to disable interrupts on
 */
static void iecm_intr_rel(struct iecm_adapter *adapter)
{
	if (!adapter->msix_entries)
		return;
	clear_bit(__IECM_MB_INTR_MODE, adapter->flags);
	clear_bit(__IECM_MB_INTR_TRIGGER, adapter->flags);
	iecm_mb_intr_rel_irq(adapter);

	pci_free_irq_vectors(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
	kfree(adapter->req_vec_chunks);
	adapter->req_vec_chunks = NULL;
}

/**
 * iecm_mb_intr_clean - Interrupt handler for the mailbox
 * @irq: interrupt number
 * @data: pointer to the adapter structure
 */
static irqreturn_t iecm_mb_intr_clean(int __always_unused irq, void *data)
{
	struct iecm_adapter *adapter = (struct iecm_adapter *)data;

	set_bit(__IECM_MB_INTR_TRIGGER, adapter->flags);
	queue_delayed_work(adapter->serv_wq, &adapter->serv_task,
			   msecs_to_jiffies(0));
	return IRQ_HANDLED;
}

/**
 * iecm_mb_irq_enable - Enable MSIX interrupt for the mailbox
 * @adapter: adapter to get the hardware address for register write
 */
static void iecm_mb_irq_enable(struct iecm_adapter *adapter)
{
	struct iecm_hw *hw = &adapter->hw;
	struct iecm_intr_reg *intr = &adapter->mb_vector.intr_reg;
	u32 val;

	val = intr->dyn_ctl_intena_m | intr->dyn_ctl_itridx_m;
	wr32(hw, intr->dyn_ctl, val);
}

/**
 * iecm_mb_intr_req_irq - Request IRQ for the mailbox interrupt
 * @adapter: adapter structure to pass to the mailbox IRQ handler
 */
static int iecm_mb_intr_req_irq(struct iecm_adapter *adapter)
{
	struct iecm_q_vector *mb_vector = &adapter->mb_vector;
	int irq_num, mb_vidx = 0, err;

	irq_num = adapter->msix_entries[mb_vidx].vector;
	snprintf(mb_vector->name, sizeof(mb_vector->name) - 1,
		 "%s-%s-%d", dev_driver_string(&adapter->pdev->dev),
		 "Mailbox", mb_vidx);
	err = request_irq(irq_num, adapter->irq_mb_handler, 0,
			  mb_vector->name, adapter);
	if (err) {
		dev_err(&adapter->pdev->dev,
			"Request_irq for mailbox failed, error: %d\n", err);
		return err;
	}
	set_bit(__IECM_MB_INTR_MODE, adapter->flags);
	return 0;
}

/**
 * iecm_get_mb_vec_id - Get vector index for mailbox
 * @adapter: adapter structure to access the vector chunks
 *
 * The first vector id in the requested vector chunks from the CP is for
 * the mailbox
 */
static void iecm_get_mb_vec_id(struct iecm_adapter *adapter)
{
	struct virtchnl_vector_chunks *vchunks;
	struct virtchnl_vector_chunk *chunk;

	if (adapter->req_vec_chunks) {
		vchunks = &adapter->req_vec_chunks->vchunks;
		chunk = &vchunks->num_vchunk[0];
		adapter->mb_vector.v_idx = chunk->start_vector_id;
	} else {
		adapter->mb_vector.v_idx = 0;
	}
}

/**
 * iecm_mb_intr_init - Initialize the mailbox interrupt
 * @adapter: adapter structure to store the mailbox vector
 */
static int iecm_mb_intr_init(struct iecm_adapter *adapter)
{
	iecm_get_mb_vec_id(adapter);
	adapter->dev_ops.reg_ops.mb_intr_reg_init(adapter);
	adapter->irq_mb_handler = iecm_mb_intr_clean;
	return iecm_mb_intr_req_irq(adapter);
}

/**
 * iecm_intr_distribute - Distribute MSIX vectors
 * @adapter: adapter structure to get the vports
 *
 * Distribute the MSIX vectors acquired from the OS to the vports based on the
 * num of vectors requested by each vport
 */
static void iecm_intr_distribute(struct iecm_adapter *adapter)
{
	struct iecm_vport *vport;

	vport = adapter->vports[0];
	if (adapter->num_msix_entries != adapter->num_req_msix)
		vport->num_q_vectors = adapter->num_msix_entries -
				       IECM_MAX_NONQ_VEC - IECM_MIN_RDMA_VEC;
}

/**
 * iecm_intr_req - Request interrupt capabilities
 * @adapter: adapter to enable interrupts on
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_intr_req(struct iecm_adapter *adapter)
{
	int min_vectors, max_vectors, err = 0;
	unsigned int vector;
	int num_vecs;
	int v_actual;

	num_vecs = adapter->vports[0]->num_q_vectors +
		   IECM_MAX_NONQ_VEC + IECM_MAX_RDMA_VEC;

	min_vectors = IECM_MIN_VEC;
#define IECM_MAX_EVV_MAPPED_VEC 16
	max_vectors = min(num_vecs, IECM_MAX_EVV_MAPPED_VEC);

	v_actual = pci_alloc_irq_vectors(adapter->pdev, min_vectors,
					 max_vectors, PCI_IRQ_MSIX);
	if (v_actual < 0) {
		dev_err(&adapter->pdev->dev, "Failed to allocate MSIX vectors: %d\n",
			v_actual);
		return v_actual;
	}

	adapter->msix_entries = kcalloc(v_actual, sizeof(struct msix_entry),
					GFP_KERNEL);

	if (!adapter->msix_entries) {
		pci_free_irq_vectors(adapter->pdev);
		return -ENOMEM;
	}

	for (vector = 0; vector < v_actual; vector++) {
		adapter->msix_entries[vector].entry = vector;
		adapter->msix_entries[vector].vector =
			pci_irq_vector(adapter->pdev, vector);
	}
	adapter->num_msix_entries = v_actual;
	adapter->num_req_msix = num_vecs;

	iecm_intr_distribute(adapter);

	err = iecm_mb_intr_init(adapter);
	if (err)
		goto intr_rel;
	iecm_mb_irq_enable(adapter);
	return err;

intr_rel:
	iecm_intr_rel(adapter);
	return err;
}

/**
 * iecm_cfg_netdev - Allocate, configure and register a netdev
 * @vport: main vport structure
 *
 * Returns 0 on success, negative value on failure
 */
static int iecm_cfg_netdev(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	netdev_features_t dflt_features;
	netdev_features_t offloads = 0;
	struct iecm_netdev_priv *np;
	struct net_device *netdev;
	int err;

	netdev = alloc_etherdev_mqs(sizeof(struct iecm_netdev_priv),
				    IECM_MAX_Q, IECM_MAX_Q);
	if (!netdev)
		return -ENOMEM;
	vport->netdev = netdev;
	np = netdev_priv(netdev);
	np->vport = vport;

	if (!is_valid_ether_addr(vport->default_mac_addr)) {
		eth_hw_addr_random(netdev);
		ether_addr_copy(vport->default_mac_addr, netdev->dev_addr);
	} else {
		ether_addr_copy(netdev->dev_addr, vport->default_mac_addr);
		ether_addr_copy(netdev->perm_addr, vport->default_mac_addr);
	}

	/* assign netdev_ops */
	if (iecm_is_queue_model_split(vport->txq_model))
		netdev->netdev_ops = &iecm_netdev_ops_splitq;
	else
		netdev->netdev_ops = &iecm_netdev_ops_singleq;

	/* setup watchdog timeout value to be 5 second */
	netdev->watchdog_timeo = 5 * HZ;

	/* configure default MTU size */
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = vport->max_mtu;

	dflt_features = NETIF_F_SG	|
			NETIF_F_HIGHDMA	|
			NETIF_F_RXHASH;

	if (iecm_is_cap_ena(adapter, VIRTCHNL_CAP_STATELESS_OFFLOADS)) {
		dflt_features |=
			NETIF_F_RXCSUM |
			NETIF_F_IP_CSUM |
			NETIF_F_IPV6_CSUM |
			0;
		offloads |= NETIF_F_TSO |
			    NETIF_F_TSO6;
	}
	if (iecm_is_cap_ena(adapter, VIRTCHNL_CAP_UDP_SEG_OFFLOAD))
		offloads |= NETIF_F_GSO_UDP_L4;

	netdev->features |= dflt_features;
	netdev->hw_features |= dflt_features | offloads;
	netdev->hw_enc_features |= dflt_features | offloads;

	iecm_set_ethtool_ops(netdev);
	SET_NETDEV_DEV(netdev, &adapter->pdev->dev);

	/* carrier off on init to avoid Tx hangs */
	netif_carrier_off(netdev);

	/* make sure transmit queues start off as stopped */
	netif_tx_stop_all_queues(netdev);

	/* register last */
	err = register_netdev(netdev);
	if (err)
		goto err;

	return 0;
err:
	free_netdev(vport->netdev);
	vport->netdev = NULL;

	return err;
}

/**
 * iecm_cfg_hw - Initialize HW struct
 * @adapter: adapter to setup hw struct for
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_cfg_hw(struct iecm_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	struct iecm_hw *hw = &adapter->hw;

	hw->hw_addr_len = pci_resource_len(pdev, 0);
	hw->hw_addr = ioremap(pci_resource_start(pdev, 0), hw->hw_addr_len);

	if (!hw->hw_addr)
		return -EIO;

	hw->back = adapter;

	return 0;
}

/**
 * iecm_vport_res_alloc - Allocate vport major memory resources
 * @vport: virtual port private structure
 */
static int iecm_vport_res_alloc(struct iecm_vport *vport)
{
	if (iecm_vport_queues_alloc(vport))
		return -ENOMEM;

	if (iecm_vport_intr_alloc(vport)) {
		iecm_vport_queues_rel(vport);
		return -ENOMEM;
	}

	return 0;
}

/**
 * iecm_vport_res_free - Free vport major memory resources
 * @vport: virtual port private structure
 */
static void iecm_vport_res_free(struct iecm_vport *vport)
{
	iecm_vport_intr_rel(vport);
	iecm_vport_queues_rel(vport);
}

/**
 * iecm_get_free_slot - get the next non-NULL location index in array
 * @array: array to search
 * @size: size of the array
 * @curr: last known occupied index to be used as a search hint
 *
 * void * is being used to keep the functionality generic. This lets us use this
 * function on any array of pointers.
 */
static int iecm_get_free_slot(void *array, int size, int curr)
{
	int **tmp_array = (int **)array;
	int next;

	if (curr < (size - 1) && !tmp_array[curr + 1]) {
		next = curr + 1;
	} else {
		int i = 0;

		while ((i < size) && (tmp_array[i]))
			i++;
		if (i == size)
			next = IECM_NO_FREE_SLOT;
		else
			next = i;
	}
	return next;
}

/**
 * iecm_netdev_to_vport - get a vport handle from a netdev
 * @netdev: network interface device structure
 */
struct iecm_vport *iecm_netdev_to_vport(struct net_device *netdev)
{
	struct iecm_netdev_priv *np = netdev_priv(netdev);

	return np->vport;
}

/**
 * iecm_netdev_to_adapter - get an adapter handle from a netdev
 * @netdev: network interface device structure
 */
struct iecm_adapter *iecm_netdev_to_adapter(struct net_device *netdev)
{
	struct iecm_netdev_priv *np = netdev_priv(netdev);

	return np->vport->adapter;
}

/**
 * iecm_vport_stop - Disable a vport
 * @vport: vport to disable
 */
static void iecm_vport_stop(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_stop - Disables a network interface
 * @netdev: network interface device structure
 *
 * The stop entry point is called when an interface is de-activated by the OS,
 * and the netdevice enters the DOWN state.  The hardware is still under the
 * driver's control, but the netdev interface is disabled.
 *
 * Returns success only - not allowed to fail
 */
static int iecm_stop(struct net_device *netdev)
{
	/* stub */
}

/**
 * iecm_vport_rel - Delete a vport and free its resources
 * @vport: the vport being removed
 */
static void iecm_vport_rel(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;

	iecm_vport_stop(vport);
	iecm_vport_res_free(vport);
	iecm_deinit_rss(vport);
	unregister_netdev(vport->netdev);
	free_netdev(vport->netdev);
	vport->netdev = NULL;
	if (adapter->dev_ops.vc_ops.destroy_vport)
		adapter->dev_ops.vc_ops.destroy_vport(vport);
	kfree(vport);
}

/**
 * iecm_vport_rel_all - Delete all vports
 * @adapter: adapter from which all vports are being removed
 */
static void iecm_vport_rel_all(struct iecm_adapter *adapter)
{
	int i;

	if (!adapter->vports)
		return;

	for (i = 0; i < adapter->num_alloc_vport; i++) {
		if (!adapter->vports[i])
			continue;

		iecm_vport_rel(adapter->vports[i]);
		adapter->vports[i] = NULL;
	}
	adapter->num_alloc_vport = 0;
}

/**
 * iecm_vport_set_hsplit - enable or disable header split on a given vport
 * @vport: virtual port
 * @prog: bpf_program attached to an interface or NULL
 */
void iecm_vport_set_hsplit(struct iecm_vport *vport,
			   struct bpf_prog __always_unused *prog)
{
	if (prog) {
		vport->rx_hsplit_en = IECM_RX_NO_HDR_SPLIT;
		return;
	}
	if (iecm_is_cap_ena(vport->adapter, VIRTCHNL_CAP_HEADER_SPLIT) &&
	    iecm_is_queue_model_split(vport->rxq_model))
		vport->rx_hsplit_en = IECM_RX_HDR_SPLIT;
	else
		vport->rx_hsplit_en = IECM_RX_NO_HDR_SPLIT;
}

/**
 * iecm_vport_alloc - Allocates the next available struct vport in the adapter
 * @adapter: board private structure
 * @vport_id: vport identifier
 *
 * returns a pointer to a vport on success, NULL on failure.
 */
static struct iecm_vport *
iecm_vport_alloc(struct iecm_adapter *adapter, int vport_id)
{
	struct iecm_vport *vport = NULL;

	if (adapter->next_vport == IECM_NO_FREE_SLOT)
		return vport;

	/* Need to protect the allocation of the vports at the adapter level */
	mutex_lock(&adapter->sw_mutex);

	vport = kzalloc(sizeof(*vport), GFP_KERNEL);
	if (!vport)
		goto unlock_adapter;

	vport->adapter = adapter;
	vport->idx = adapter->next_vport;
	vport->compln_clean_budget = IECM_TX_COMPLQ_CLEAN_BUDGET;
	adapter->num_alloc_vport++;
	adapter->dev_ops.vc_ops.vport_init(vport, vport_id);

	/* Setup default MSIX irq handler for the vport */
	vport->irq_q_handler = iecm_vport_intr_clean_queues;
	vport->q_vector_base = IECM_MAX_NONQ_VEC;

	/* fill vport slot in the adapter struct */
	adapter->vports[adapter->next_vport] = vport;
	if (iecm_cfg_netdev(vport))
		goto cfg_netdev_fail;

	/* prepare adapter->next_vport for next use */
	adapter->next_vport = iecm_get_free_slot(adapter->vports,
						 adapter->num_alloc_vport,
						 adapter->next_vport);

	goto unlock_adapter;

cfg_netdev_fail:
	adapter->vports[adapter->next_vport] = NULL;
	kfree(vport);
	vport = NULL;
unlock_adapter:
	mutex_unlock(&adapter->sw_mutex);
	return vport;
}

/**
 * iecm_statistics_task - Delayed task to get statistics over mailbox
 * @work: work_struct handle to our data
 */
static void iecm_statistics_task(struct work_struct *work)
{
	struct iecm_adapter *adapter = container_of(work,
						    struct iecm_adapter,
						    stats_task.work);

	iecm_send_get_stats_msg(adapter->vports[0]);
	queue_delayed_work(adapter->stats_wq, &adapter->stats_task,
			   msecs_to_jiffies(1000));
}

/**
 * iecm_service_task - Delayed task for handling mailbox responses
 * @work: work_struct handle to our data
 *
 */
static void iecm_service_task(struct work_struct *work)
{
	struct iecm_adapter *adapter = container_of(work,
						    struct iecm_adapter,
						    serv_task.work);

	if (test_bit(__IECM_MB_INTR_MODE, adapter->flags)) {
		if (test_and_clear_bit(__IECM_MB_INTR_TRIGGER,
				       adapter->flags)) {
			iecm_recv_mb_msg(adapter, VIRTCHNL_OP_UNKNOWN, NULL, 0);
			iecm_mb_irq_enable(adapter);
		}
	} else {
		iecm_recv_mb_msg(adapter, VIRTCHNL_OP_UNKNOWN, NULL, 0);
	}

	queue_delayed_work(adapter->serv_wq, &adapter->serv_task,
			   msecs_to_jiffies(300));
}

/**
 * iecm_up_complete - Complete interface up sequence
 * @vport: virtual port structure
 */
static int iecm_up_complete(struct iecm_vport *vport)
{
	int err;

	err = netif_set_real_num_rx_queues(vport->netdev, vport->num_txq);
	if (err)
		return err;
	err = netif_set_real_num_tx_queues(vport->netdev, vport->num_rxq);
	if (err)
		return err;
	netif_carrier_on(vport->netdev);
	netif_tx_start_all_queues(vport->netdev);

	vport->adapter->state = __IECM_UP;
	return 0;
}

/**
 * iecm_rx_init_buf_tail - Write initial buffer ring tail value
 * @vport: virtual port struct
 */
static void iecm_rx_init_buf_tail(struct iecm_vport *vport)
{
	int i, j;

	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *grp = &vport->rxq_grps[i];

		if (iecm_is_queue_model_split(vport->rxq_model)) {
			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++) {
				struct iecm_queue *q =
					&grp->splitq.bufq_sets[j].bufq;

				writel(q->next_to_alloc, q->tail);
			}
		} else {
			for (j = 0; j < grp->singleq.num_rxq; j++) {
				struct iecm_queue *q =
					&grp->singleq.rxqs[j];

				writel(q->next_to_alloc, q->tail);
			}
		}
	}
}

/**
 * iecm_vport_open - Bring up a vport
 * @vport: vport to bring up
 */
static int iecm_vport_open(struct iecm_vport *vport)
{
	struct iecm_adapter *adapter = vport->adapter;
	int err;

	if (vport->adapter->state != __IECM_DOWN)
		return -EBUSY;

	/* we do not allow interface up just yet */
	netif_carrier_off(vport->netdev);

	if (adapter->dev_ops.vc_ops.enable_vport) {
		err = adapter->dev_ops.vc_ops.enable_vport(vport);
		if (err)
			return -EAGAIN;
	}

	err = adapter->dev_ops.vc_ops.vport_queue_ids_init(vport);
	if (err) {
		dev_err(&vport->adapter->pdev->dev,
			"Call to queue ids init returned %d\n", err);
		return err;
	}

	adapter->dev_ops.reg_ops.vportq_reg_init(vport);
	iecm_rx_init_buf_tail(vport);

	err = iecm_vport_intr_init(vport);
	if (err) {
		dev_err(&vport->adapter->pdev->dev,
			"Call to vport interrupt init returned %d\n", err);
		return err;
	}

	err = vport->adapter->dev_ops.vc_ops.config_queues(vport);
	if (err)
		goto unroll_config_queues;
	err = vport->adapter->dev_ops.vc_ops.irq_map_unmap(vport, true);
	if (err) {
		dev_err(&vport->adapter->pdev->dev,
			"Call to irq_map_unmap returned %d\n", err);
		goto unroll_config_queues;
	}
	err = vport->adapter->dev_ops.vc_ops.enable_queues(vport);
	if (err)
		goto unroll_enable_queues;

	err = vport->adapter->dev_ops.vc_ops.get_ptype(vport);
	if (err)
		goto unroll_get_ptype;

	if (adapter->rss_data.rss_lut)
		err = iecm_config_rss(vport);
	else
		err = iecm_init_rss(vport);
	if (err)
		goto unroll_init_rss;
	err = iecm_up_complete(vport);
	if (err)
		goto unroll_up_comp;

	netif_info(vport->adapter, hw, vport->netdev, "%s\n", __func__);

	return 0;
unroll_up_comp:
	iecm_deinit_rss(vport);
unroll_init_rss:
	adapter->dev_ops.vc_ops.disable_vport(vport);
unroll_get_ptype:
	vport->adapter->dev_ops.vc_ops.disable_queues(vport);
unroll_enable_queues:
	vport->adapter->dev_ops.vc_ops.irq_map_unmap(vport, false);
unroll_config_queues:
	iecm_vport_intr_deinit(vport);

	return err;
}

/**
 * iecm_init_task - Delayed initialization task
 * @work: work_struct handle to our data
 *
 * Init task finishes up pending work started in probe.  Due to the asynchronous
 * nature in which the device communicates with hardware, we may have to wait
 * several milliseconds to get a response.  Instead of busy polling in probe,
 * pulling it out into a delayed work task prevents us from bogging down the
 * whole system waiting for a response from hardware.
 */
static void iecm_init_task(struct work_struct *work)
{
	struct iecm_adapter *adapter = container_of(work,
						    struct iecm_adapter,
						    init_task.work);
	struct iecm_vport *vport;
	struct pci_dev *pdev;
	int vport_id, err;

	err = adapter->dev_ops.vc_ops.core_init(adapter, &vport_id);
	if (err)
		return;

	pdev = adapter->pdev;
	vport = iecm_vport_alloc(adapter, vport_id);
	if (!vport) {
		err = -EFAULT;
		dev_err(&pdev->dev, "probe failed on vport setup:%d\n",
			err);
		return;
	}
	/* Start the service task before requesting vectors. This will ensure
	 * vector information response from mailbox is handled
	 */
	queue_delayed_work(adapter->serv_wq, &adapter->serv_task,
			   msecs_to_jiffies(5 * (pdev->devfn & 0x07)));
	err = iecm_intr_req(adapter);
	if (err) {
		dev_err(&pdev->dev, "failed to enable interrupt vectors: %d\n",
			err);
		iecm_vport_rel(vport);
		return;
	}
	/* Deal with major memory allocations for vport resources */
	err = iecm_vport_res_alloc(vport);
	if (err) {
		dev_err(&pdev->dev, "failed to allocate resources: %d\n",
			err);
		iecm_vport_rel(vport);
		return;
	}

	queue_delayed_work(adapter->stats_wq, &adapter->stats_task,
			   msecs_to_jiffies(10 * (pdev->devfn & 0x07)));
	/* Once state is put into DOWN, driver is ready for dev_open */
	adapter->state = __IECM_DOWN;
	if (test_and_clear_bit(__IECM_UP_REQUESTED, adapter->flags))
		iecm_vport_open(vport);
}

/**
 * iecm_api_init - Initialize and verify device API
 * @adapter: driver specific private structure
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_api_init(struct iecm_adapter *adapter)
{
	struct iecm_reg_ops *reg_ops = &adapter->dev_ops.reg_ops;
	struct pci_dev *pdev = adapter->pdev;

	if (!adapter->dev_ops.reg_ops_init) {
		dev_err(&pdev->dev, "Invalid device, register API init not defined\n");
		return -EINVAL;
	}
	adapter->dev_ops.reg_ops_init(adapter);
	if (!(reg_ops->ctlq_reg_init && reg_ops->vportq_reg_init &&
	      reg_ops->intr_reg_init && reg_ops->mb_intr_reg_init &&
	      reg_ops->reset_reg_init && reg_ops->trigger_reset)) {
		dev_err(&pdev->dev, "Invalid device, missing one or more register functions\n");
		return -EINVAL;
	}

	if (adapter->dev_ops.vc_ops_init) {
		struct iecm_virtchnl_ops *vc_ops;

		adapter->dev_ops.vc_ops_init(adapter);
		vc_ops = &adapter->dev_ops.vc_ops;
		if (!(vc_ops->core_init &&
		      vc_ops->vport_init &&
		      vc_ops->vport_queue_ids_init &&
		      vc_ops->get_caps &&
		      vc_ops->config_queues &&
		      vc_ops->enable_queues &&
		      vc_ops->disable_queues &&
		      vc_ops->irq_map_unmap &&
		      vc_ops->get_set_rss_lut &&
		      vc_ops->get_set_rss_hash &&
		      vc_ops->adjust_qs &&
		      vc_ops->get_ptype)) {
			dev_err(&pdev->dev, "Invalid device, missing one or more virtchnl functions\n");
			return -EINVAL;
		}
	} else {
		iecm_vc_ops_init(adapter);
	}

	return 0;
}

/**
 * iecm_deinit_task - Device deinit routine
 * @adapter: Driver specific private structure
 *
 * Extended remove logic which will be used for
 * hard reset as well
 */
static void iecm_deinit_task(struct iecm_adapter *adapter)
{
	iecm_vport_rel_all(adapter);
	cancel_delayed_work_sync(&adapter->serv_task);
	cancel_delayed_work_sync(&adapter->stats_task);
	iecm_deinit_dflt_mbx(adapter);
	iecm_vport_params_buf_rel(adapter);
	iecm_intr_rel(adapter);
}

/**
 * iecm_init_hard_reset - Initiate a hardware reset
 * @adapter: Driver specific private structure
 *
 * Deallocate the vports and all the resources associated with them and
 * reallocate. Also reinitialize the mailbox.	Return 0 on success,
 * negative on failure.
 */
static int iecm_init_hard_reset(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_vc_event_task - Handle virtchannel event logic
 * @work: work queue struct
 */
static void iecm_vc_event_task(struct work_struct *work)
{
	struct iecm_adapter *adapter = container_of(work,
						    struct iecm_adapter,
						    vc_event_task.work);

	if (test_bit(__IECM_HR_CORE_RESET, adapter->flags) ||
	    test_bit(__IECM_HR_FUNC_RESET, adapter->flags))
		iecm_init_hard_reset(adapter);
}

/**
 * iecm_initiate_soft_reset - Initiate a software reset
 * @vport: virtual port data struct
 * @reset_cause: reason for the soft reset
 *
 * Soft reset only reallocs vport queue resources. Returns 0 on success,
 * negative on failure.
 */
int iecm_initiate_soft_reset(struct iecm_vport *vport,
			     enum iecm_flags reset_cause)
{
	/* stub */
}

/**
 * iecm_probe - Device initialization routine
 * @pdev: PCI device information struct
 * @ent: entry in iecm_pci_tbl
 * @adapter: driver specific private structure
 *
 * Returns 0 on success, negative on failure
 */
int iecm_probe(struct pci_dev *pdev,
	       const struct pci_device_id __always_unused *ent,
	       struct iecm_adapter *adapter)
{
	int err;

	adapter->pdev = pdev;
	err = iecm_api_init(adapter);
	if (err) {
		dev_err(&pdev->dev, "Device API is incorrectly configured\n");
		return err;
	}

	err = pcim_iomap_regions(pdev, BIT(IECM_BAR0), pci_name(pdev));
	if (err) {
		dev_err(&pdev->dev, "BAR0 I/O map error %d\n", err);
		return err;
	}

	/* set up for high or low DMA */
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (err)
		err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (err) {
		dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
		return err;
	}

	pci_enable_pcie_error_reporting(pdev);
	pci_set_master(pdev);
	pci_set_drvdata(pdev, adapter);

	adapter->init_wq =
		alloc_workqueue("%s", WQ_MEM_RECLAIM, 0, KBUILD_MODNAME);
	if (!adapter->init_wq) {
		dev_err(&pdev->dev, "Failed to allocate workqueue\n");
		err = -ENOMEM;
		goto err_wq_alloc;
	}

	adapter->serv_wq =
		alloc_workqueue("%s", WQ_MEM_RECLAIM, 0, KBUILD_MODNAME);
	if (!adapter->serv_wq) {
		dev_err(&pdev->dev, "Failed to allocate workqueue\n");
		err = -ENOMEM;
		goto err_mbx_wq_alloc;
	}

	adapter->stats_wq =
		alloc_workqueue("%s", WQ_MEM_RECLAIM, 0, KBUILD_MODNAME);
	if (!adapter->stats_wq) {
		dev_err(&pdev->dev, "Failed to allocate workqueue\n");
		err = -ENOMEM;
		goto err_stats_wq_alloc;
	}

	adapter->vc_event_wq =
		alloc_workqueue("%s", WQ_MEM_RECLAIM, 0, KBUILD_MODNAME);
	if (!adapter->vc_event_wq) {
		dev_err(&pdev->dev, "Failed to allocate workqueue\n");
		err = -ENOMEM;
		goto err_vc_event_wq_alloc;
	}

	/* setup msglvl */
	adapter->msg_enable = netif_msg_init(adapter->debug_msk,
					     IECM_AVAIL_NETIF_M);

	adapter->vports = kcalloc(IECM_MAX_NUM_VPORTS,
				  sizeof(*adapter->vports), GFP_KERNEL);
	if (!adapter->vports) {
		err = -ENOMEM;
		goto err_vport_alloc;
	}

	err = iecm_vport_params_buf_alloc(adapter);
	if (err) {
		dev_err(&pdev->dev, "Failed to alloc vport params buffer: %d\n",
			err);
		goto err_mb_res;
	}

	err = iecm_cfg_hw(adapter);
	if (err) {
		dev_err(&pdev->dev, "Failed to configure HW structure for adapter: %d\n",
			err);
		goto err_cfg_hw;
	}

	mutex_init(&adapter->sw_mutex);
	mutex_init(&adapter->vc_msg_lock);
	mutex_init(&adapter->reset_lock);
	init_waitqueue_head(&adapter->vchnl_wq);

	INIT_DELAYED_WORK(&adapter->stats_task, iecm_statistics_task);
	INIT_DELAYED_WORK(&adapter->serv_task, iecm_service_task);
	INIT_DELAYED_WORK(&adapter->init_task, iecm_init_task);
	INIT_DELAYED_WORK(&adapter->vc_event_task, iecm_vc_event_task);

	mutex_lock(&adapter->reset_lock);
	set_bit(__IECM_HR_DRV_LOAD, adapter->flags);
	err = iecm_init_hard_reset(adapter);
	if (err) {
		dev_err(&pdev->dev, "Failed to reset device: %d\n", err);
		goto err_mb_init;
	}

	return 0;
err_mb_init:
err_cfg_hw:
	iecm_vport_params_buf_rel(adapter);
err_mb_res:
	kfree(adapter->vports);
err_vport_alloc:
	destroy_workqueue(adapter->vc_event_wq);
err_vc_event_wq_alloc:
	destroy_workqueue(adapter->stats_wq);
err_stats_wq_alloc:
	destroy_workqueue(adapter->serv_wq);
err_mbx_wq_alloc:
	destroy_workqueue(adapter->init_wq);
err_wq_alloc:
	pci_disable_pcie_error_reporting(pdev);
	return err;
}
EXPORT_SYMBOL(iecm_probe);

/**
 * iecm_remove - Device removal routine
 * @pdev: PCI device information struct
 */
void iecm_remove(struct pci_dev *pdev)
{
	struct iecm_adapter *adapter = pci_get_drvdata(pdev);

	if (!adapter)
		return;

	iecm_deinit_task(adapter);
	cancel_delayed_work_sync(&adapter->vc_event_task);
	destroy_workqueue(adapter->serv_wq);
	destroy_workqueue(adapter->init_wq);
	destroy_workqueue(adapter->stats_wq);
	kfree(adapter->vports);
	kfree(adapter->vport_params_recvd);
	kfree(adapter->vport_params_reqd);
	mutex_destroy(&adapter->sw_mutex);
	mutex_destroy(&adapter->vc_msg_lock);
	mutex_destroy(&adapter->reset_lock);
	pci_disable_pcie_error_reporting(pdev);
}
EXPORT_SYMBOL(iecm_remove);

/**
 * iecm_shutdown - PCI callback for shutting down device
 * @pdev: PCI device information struct
 */
void iecm_shutdown(struct pci_dev *pdev)
{
	struct iecm_adapter *adapter;

	adapter = pci_get_drvdata(pdev);
	adapter->state = __IECM_REMOVE;

	if (system_state == SYSTEM_POWER_OFF)
		pci_set_power_state(pdev, PCI_D3hot);
}
EXPORT_SYMBOL(iecm_shutdown);

/**
 * iecm_open - Called when a network interface becomes active
 * @netdev: network interface device structure
 *
 * The open entry point is called when a network interface is made
 * active by the system (IFF_UP).  At this point all resources needed
 * for transmit and receive operations are allocated, the interrupt
 * handler is registered with the OS, the netdev watchdog is enabled,
 * and the stack is notified that the interface is ready.
 *
 * Returns 0 on success, negative value on failure
 */
static int iecm_open(struct net_device *netdev)
{
	struct iecm_netdev_priv *np = netdev_priv(netdev);

	return iecm_vport_open(np->vport);
}

/**
 * iecm_change_mtu - NDO callback to change the MTU
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_change_mtu(struct net_device *netdev, int new_mtu)
{
	/* stub */
}

void *iecm_alloc_dma_mem(struct iecm_hw *hw, struct iecm_dma_mem *mem, u64 size)
{
	size_t sz = ALIGN(size, 4096);
	struct iecm_adapter *pf = hw->back;

	mem->va = dma_alloc_coherent(&pf->pdev->dev, sz,
				     &mem->pa, GFP_KERNEL | __GFP_ZERO);
	mem->size = size;

	return mem->va;
}

void iecm_free_dma_mem(struct iecm_hw *hw, struct iecm_dma_mem *mem)
{
	struct iecm_adapter *pf = hw->back;

	dma_free_coherent(&pf->pdev->dev, mem->size,
			  mem->va, mem->pa);
	mem->size = 0;
	mem->va = NULL;
	mem->pa = 0;
}

static const struct net_device_ops iecm_netdev_ops_splitq = {
	.ndo_open = iecm_open,
	.ndo_stop = iecm_stop,
	.ndo_start_xmit = iecm_tx_splitq_start,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_get_stats64 = iecm_get_stats64,
};

static const struct net_device_ops iecm_netdev_ops_singleq = {
	.ndo_open = iecm_open,
	.ndo_stop = iecm_stop,
	.ndo_start_xmit = iecm_tx_singleq_start,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_change_mtu = iecm_change_mtu,
	.ndo_get_stats64 = iecm_get_stats64,
};
