/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019, Intel Corporation. */

#ifndef _IIDC_H_
#define _IIDC_H_

#include <linux/dcbnl.h>
#include <linux/device.h>
#include <linux/if_ether.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/virtual_bus.h>

enum iidc_event_type {
	IIDC_EVENT_LINK_CHANGE,
	IIDC_EVENT_MTU_CHANGE,
	IIDC_EVENT_TC_CHANGE,
	IIDC_EVENT_API_CHANGE,
	IIDC_EVENT_MBX_CHANGE,
	IIDC_EVENT_NBITS		/* must be last */
};

enum iidc_res_type {
	IIDC_INVAL_RES,
	IIDC_VSI,
	IIDC_VEB,
	IIDC_EVENT_Q,
	IIDC_EGRESS_CMPL_Q,
	IIDC_CMPL_EVENT_Q,
	IIDC_ASYNC_EVENT_Q,
	IIDC_DOORBELL_Q,
	IIDC_RDMA_QSETS_TXSCHED,
};

enum iidc_peer_reset_type {
	IIDC_PEER_PFR,
	IIDC_PEER_CORER,
	IIDC_PEER_CORER_SW_CORE,
	IIDC_PEER_CORER_SW_FULL,
	IIDC_PEER_GLOBR,
};

/* reason notified to peer driver as part of event handling */
enum iidc_close_reason {
	IIDC_REASON_INVAL,
	IIDC_REASON_HW_UNRESPONSIVE,
	IIDC_REASON_INTERFACE_DOWN, /* Administrative down */
	IIDC_REASON_PEER_DRV_UNREG, /* peer driver getting unregistered */
	IIDC_REASON_PEER_DEV_UNINIT,
	IIDC_REASON_GLOBR_REQ,
	IIDC_REASON_CORER_REQ,
	/* Reason #7 reserved */
	IIDC_REASON_PFR_REQ = 8,
	IIDC_REASON_HW_RESET_PENDING,
	IIDC_REASON_RECOVERY_MODE,
	IIDC_REASON_PARAM_CHANGE,
};

enum iidc_rdma_filter {
	IIDC_RDMA_FILTER_INVAL,
	IIDC_RDMA_FILTER_IWARP,
	IIDC_RDMA_FILTER_ROCEV2,
	IIDC_RDMA_FILTER_BOTH,
};

/* Struct to hold per DCB APP info */
struct iidc_dcb_app_info {
	u8  priority;
	u8  selector;
	u16 prot_id;
};

struct iidc_peer_dev;

#define IIDC_MAX_USER_PRIORITY		8
#define IIDC_MAX_APPS			8

/* Struct to hold per RDMA Qset info */
struct iidc_rdma_qset_params {
	u32 teid;	/* qset TEID */
	u16 qs_handle; /* RDMA driver provides this */
	u16 vsi_id; /* VSI index */
	u8 tc; /* TC branch the QSet should belong to */
	u8 reserved[3];
};

struct iidc_res_base {
	/* Union for future provision e.g. other res_type */
	union {
		struct iidc_rdma_qset_params qsets;
	} res;
};

struct iidc_res {
	/* Type of resource. Filled by peer driver */
	enum iidc_res_type res_type;
	/* Count requested by peer driver */
	u16 cnt_req;

	/* Number of resources allocated. Filled in by callee.
	 * Based on this value, caller to fill up "resources"
	 */
	u16 res_allocated;

	/* Unique handle to resources allocated. Zero if call fails.
	 * Allocated by callee and for now used by caller for internal
	 * tracking purpose.
	 */
	u32 res_handle;

	/* Peer driver has to allocate sufficient memory, to accommodate
	 * cnt_requested before calling this function.
	 * Memory has to be zero initialized. It is input/output param.
	 * As a result of alloc_res API, this structures will be populated.
	 */
	struct iidc_res_base res[1];
};

struct iidc_qos_info {
	u64 tc_ctx;
	u8 rel_bw;
	u8 prio_type;
	u8 egress_virt_up;
	u8 ingress_virt_up;
};

/* Struct to hold QoS info */
struct iidc_qos_params {
	struct iidc_qos_info tc_info[IEEE_8021QAZ_MAX_TCS];
	u8 up2tc[IIDC_MAX_USER_PRIORITY];
	u8 vsi_relative_bw;
	u8 vsi_priority_type;
	u32 num_apps;
	struct iidc_dcb_app_info apps[IIDC_MAX_APPS];
	u8 num_tc;
};

union iidc_event_info {
	/* IIDC_EVENT_LINK_CHANGE */
	struct {
		struct net_device *lwr_nd;
		u16 vsi_num; /* HW index of VSI corresponding to lwr ndev */
		u8 new_link_state;
		u8 lport;
	} link_info;
	/* IIDC_EVENT_MTU_CHANGE */
	u16 mtu;
	/* IIDC_EVENT_TC_CHANGE */
	struct iidc_qos_params port_qos;
	/* IIDC_EVENT_API_CHANGE */
	u8 api_rdy;
	/* IIDC_EVENT_MBX_CHANGE */
	u8 mbx_rdy;
};

/* iidc_event elements are to be passed back and forth between the device
 * owner and the peer drivers. They are to be used to both register/unregister
 * for event reporting and to report an event (events can be either device
 * owner generated or peer generated).
 *
 * For (un)registering for events, the structure needs to be populated with:
 *   reporter - pointer to the iidc_peer_dev struct of the peer (un)registering
 *   type - bitmap with bits set for event types to (un)register for
 *
 * For reporting events, the structure needs to be populated with:
 *   reporter - pointer to peer that generated the event (NULL for ice)
 *   type - bitmap with single bit set for this event type
 *   info - union containing data relevant to this event type
 */
struct iidc_event {
	struct iidc_peer_dev *reporter;
	DECLARE_BITMAP(type, IIDC_EVENT_NBITS);
	union iidc_event_info info;
};

/* Following APIs are implemented by device owner and invoked by peer
 * drivers
 */
struct iidc_ops {
	/* APIs to allocate resources such as VEB, VSI, Doorbell queues,
	 * completion queues, Tx/Rx queues, etc...
	 */
	int (*alloc_res)(struct iidc_peer_dev *peer_dev,
			 struct iidc_res *res,
			 int partial_acceptable);
	int (*free_res)(struct iidc_peer_dev *peer_dev,
			struct iidc_res *res);

	int (*is_vsi_ready)(struct iidc_peer_dev *peer_dev);
	int (*peer_register)(struct iidc_peer_dev *peer_dev);
	int (*peer_unregister)(struct iidc_peer_dev *peer_dev);
	int (*request_reset)(struct iidc_peer_dev *dev,
			     enum iidc_peer_reset_type reset_type);

	void (*notify_state_change)(struct iidc_peer_dev *dev,
				    struct iidc_event *event);

	/* Notification APIs */
	void (*reg_for_notification)(struct iidc_peer_dev *dev,
				     struct iidc_event *event);
	void (*unreg_for_notification)(struct iidc_peer_dev *dev,
				       struct iidc_event *event);
	int (*update_vsi_filter)(struct iidc_peer_dev *peer_dev,
				 enum iidc_rdma_filter filter, bool enable);
	int (*vc_send)(struct iidc_peer_dev *peer_dev, u32 vf_id, u8 *msg,
		       u16 len);
};

/* Following APIs are implemented by peer drivers and invoked by device
 * owner
 */
struct iidc_peer_ops {
	void (*event_handler)(struct iidc_peer_dev *peer_dev,
			      struct iidc_event *event);

	/* Why we have 'open' and when it is expected to be called:
	 * 1. symmetric set of API w.r.t close
	 * 2. To be invoked form driver initialization path
	 *     - call peer_driver:open once device owner is fully
	 *     initialized
	 * 3. To be invoked upon RESET complete
	 */
	int (*open)(struct iidc_peer_dev *peer_dev);

	/* Peer's close function is to be called when the peer needs to be
	 * quiesced. This can be for a variety of reasons (enumerated in the
	 * iidc_close_reason enum struct). A call to close will only be
	 * followed by a call to either remove or open. No IDC calls from the
	 * peer should be accepted until it is re-opened.
	 *
	 * The *reason* parameter is the reason for the call to close. This
	 * can be for any reason enumerated in the iidc_close_reason struct.
	 * It's primary reason is for the peer's bookkeeping and in case the
	 * peer want to perform any different tasks dictated by the reason.
	 */
	void (*close)(struct iidc_peer_dev *peer_dev,
		      enum iidc_close_reason reason);

	int (*vc_receive)(struct iidc_peer_dev *peer_dev, u32 vf_id, u8 *msg,
			  u16 len);
	/* tell RDMA peer to prepare for TC change in a blocking call
	 * that will directly precede the change event
	 */
	void (*prep_tc_change)(struct iidc_peer_dev *peer_dev);
};

#define IIDC_PEER_RDMA_NAME	"intel,ice,rdma"
#define IIDC_PEER_RDMA_ID	0x00000010
#define IIDC_MAX_NUM_PEERS	4

/* The const struct that instantiates peer_dev_id needs to be initialized
 * in the .c with the macro ASSIGN_PEER_INFO.
 * For example:
 * static const struct peer_dev_id peer_dev_ids[] = ASSIGN_PEER_INFO;
 */
struct peer_dev_id {
	char *name;
	int id;
};

#define ASSIGN_PEER_INFO						\
{									\
	{ .name = IIDC_PEER_RDMA_NAME, .id = IIDC_PEER_RDMA_ID },	\
}

#define iidc_peer_priv(x) ((x)->peer_priv)

/* Structure representing peer specific information, each peer using the IIDC
 * interface will have an instance of this struct dedicated to it.
 */
struct iidc_peer_dev {
	struct pci_dev *pdev; /* PCI device of corresponding to main function */
	struct virtbus_device *vdev; /* virtual device for this peer */
	/* KVA / Linear address corresponding to BAR0 of underlying
	 * pci_device.
	 */
	u8 __iomem *hw_addr;
	int peer_dev_id;

	/* Opaque pointer for peer specific data tracking.  This memory will
	 * be alloc'd and freed by the peer driver and used for private data
	 * accessible only to the specific peer.  It is stored here so that
	 * when this struct is passed to the peer via an IDC call, the data
	 * can be accessed by the peer at that time.
	 * The peers should only retrieve the pointer by the macro:
	 *    iidc_peer_priv(struct iidc_peer_dev *)
	 */
	void *peer_priv;

	u8 ftype;	/* PF(false) or VF (true) */

	/* Data VSI created by driver */
	u16 pf_vsi_num;

	struct iidc_qos_params initial_qos_info;
	struct net_device *netdev;

	/* Based on peer driver type, this shall point to corresponding MSIx
	 * entries in pf->msix_entries (which were allocated as part of driver
	 * initialization) e.g. for RDMA driver, msix_entries reserved will be
	 * num_online_cpus + 1.
	 */
	u16 msix_count; /* How many vectors are reserved for this device */
	struct msix_entry *msix_entries;

	/* Following struct contains function pointers to be initialized
	 * by device owner and called by peer driver
	 */
	const struct iidc_ops *ops;

	/* Following struct contains function pointers to be initialized
	 * by peer driver and called by device owner
	 */
	const struct iidc_peer_ops *peer_ops;

	/* Pointer to peer_drv struct to be populated by peer driver */
	struct iidc_peer_drv *peer_drv;
};

struct iidc_virtbus_object {
	struct virtbus_device vdev;
	struct iidc_peer_dev *peer_dev;
};

/* structure representing peer driver
 * Peer driver to initialize those function ptrs and it will be invoked
 * by device owner as part of driver_registration via bus infrastructure
 */
struct iidc_peer_drv {
	u16 driver_id;
#define IIDC_PEER_DEVICE_OWNER		0
#define IIDC_PEER_RDMA_DRIVER		4

	const char *name;

};
#endif /* _IIDC_H_*/
