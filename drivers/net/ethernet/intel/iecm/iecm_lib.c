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
	/* stub */
}

/**
 * iecm_intr_rel - Release interrupt capabilities and free memory
 * @adapter: adapter to disable interrupts on
 */
static void iecm_intr_rel(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_mb_intr_clean - Interrupt handler for the mailbox
 * @irq: interrupt number
 * @data: pointer to the adapter structure
 */
static irqreturn_t iecm_mb_intr_clean(int __always_unused irq, void *data)
{
	/* stub */
}

/**
 * iecm_mb_irq_enable - Enable MSIX interrupt for the mailbox
 * @adapter: adapter to get the hardware address for register write
 */
static void iecm_mb_irq_enable(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_mb_intr_req_irq - Request IRQ for the mailbox interrupt
 * @adapter: adapter structure to pass to the mailbox IRQ handler
 */
static int iecm_mb_intr_req_irq(struct iecm_adapter *adapter)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_mb_intr_init - Initialize the mailbox interrupt
 * @adapter: adapter structure to store the mailbox vector
 */
static int iecm_mb_intr_init(struct iecm_adapter *adapter)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_intr_req - Request interrupt capabilities
 * @adapter: adapter to enable interrupts on
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_intr_req(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_cfg_netdev - Allocate, configure and register a netdev
 * @vport: main vport structure
 *
 * Returns 0 on success, negative value on failure
 */
static int iecm_cfg_netdev(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_cfg_hw - Initialize HW struct
 * @adapter: adapter to setup hw struct for
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_cfg_hw(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_vport_res_alloc - Allocate vport major memory resources
 * @vport: virtual port private structure
 */
static int iecm_vport_res_alloc(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_res_free - Free vport major memory resources
 * @vport: virtual port private structure
 */
static void iecm_vport_res_free(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_netdev_to_vport - get a vport handle from a netdev
 * @netdev: network interface device structure
 */
struct iecm_vport *iecm_netdev_to_vport(struct net_device *netdev)
{
	/* stub */
}

/**
 * iecm_netdev_to_adapter - get an adapter handle from a netdev
 * @netdev: network interface device structure
 */
struct iecm_adapter *iecm_netdev_to_adapter(struct net_device *netdev)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_vport_rel_all - Delete all vports
 * @adapter: adapter from which all vports are being removed
 */
static void iecm_vport_rel_all(struct iecm_adapter *adapter)
{
	/* stub */
}

/**
 * iecm_vport_set_hsplit - enable or disable header split on a given vport
 * @vport: virtual port
 * @prog: bpf_program attached to an interface or NULL
 */
void iecm_vport_set_hsplit(struct iecm_vport *vport,
			   struct bpf_prog __always_unused *prog)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_statistics_task - Delayed task to get statistics over mailbox
 * @work: work_struct handle to our data
 */
static void iecm_statistics_task(struct work_struct *work)
{
	 stub
}

/**
 * iecm_service_task - Delayed task for handling mailbox responses
 * @work: work_struct handle to our data
 *
 */
static void iecm_service_task(struct work_struct *work)
{
	/* stub */
}

/**
 * iecm_up_complete - Complete interface up sequence
 * @vport: virtual port structure
 */
static int iecm_up_complete(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_rx_init_buf_tail - Write initial buffer ring tail value
 * @vport: virtual port struct
 */
static void iecm_rx_init_buf_tail(struct iecm_vport *vport)
{
	/* stub */
}

/**
 * iecm_vport_open - Bring up a vport
 * @vport: vport to bring up
 */
static int iecm_vport_open(struct iecm_vport *vport)
{
	/* stub */
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
	/* stub */
}

/**
 * iecm_api_init - Initialize and verify device API
 * @adapter: driver specific private structure
 *
 * Returns 0 on success, negative on failure
 */
static int iecm_api_init(struct iecm_adapter *adapter)
{
	/* stub */
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
	/* stub */
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
	/* stub */
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
	/* stub */
}
EXPORT_SYMBOL(iecm_probe);

/**
 * iecm_remove - Device removal routine
 * @pdev: PCI device information struct
 */
void iecm_remove(struct pci_dev *pdev)
{
	/* stub */
}
EXPORT_SYMBOL(iecm_remove);

/**
 * iecm_shutdown - PCI callback for shutting down device
 * @pdev: PCI device information struct
 */
void iecm_shutdown(struct pci_dev *pdev)
{
	/* stub */
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
	/* stub */
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
	/* stub */
}

void iecm_free_dma_mem(struct iecm_hw *hw, struct iecm_dma_mem *mem)
{
	/* stub */
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
