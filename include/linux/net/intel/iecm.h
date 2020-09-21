/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2020 Intel Corporation */

#ifndef _IECM_H_
#define _IECM_H_

#include <linux/aer.h>
#include <linux/pci.h>
#include <linux/sctp.h>
#include <net/tcp.h>
#include <net/ip6_checksum.h>
#include <linux/avf/virtchnl.h>

#include "iecm_lan_txrx.h"
#include "iecm_txrx.h"
#include "iecm_controlq.h"

extern const char iecm_drv_ver[];
extern char iecm_drv_name[];

#define MAKEMASK(m, s)	((m) << (s))

#define IECM_BAR0	0
#define IECM_NO_FREE_SLOT	0xffff

/* Default Mailbox settings */
#define IECM_DFLT_MBX_BUF_SIZE	(1 * 1024)
#define IECM_NUM_QCTX_PER_MSG	3
#define IECM_DFLT_MBX_Q_LEN	64
#define IECM_DFLT_MBX_ID	-1
/* maximum number of times to try before resetting mailbox */
#define IECM_MB_MAX_ERR	20

#define IECM_MAX_NUM_VPORTS	1

/* available message levels */
#define IECM_AVAIL_NETIF_M (NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_LINK)

/* Forward declaration */
struct iecm_adapter;

enum iecm_state {
	__IECM_STARTUP,
	__IECM_VER_CHECK,
	__IECM_GET_RES,
	__IECM_GET_CAPS,
	__IECM_GET_DFLT_VPORT_PARAMS,
	__IECM_INIT_SW,
	__IECM_DOWN,
	__IECM_UP,
	__IECM_REMOVE,
	__IECM_STATE_LAST /* this member MUST be last */
};

enum iecm_flags {
	/* Soft reset causes */
	__IECM_SR_Q_CHANGE, /* Soft reset to do queue change */
	__IECM_SR_Q_DESC_CHANGE,
	__IECM_SR_Q_SCH_CHANGE, /* Scheduling mode change in queue context */
	__IECM_SR_MTU_CHANGE,
	__IECM_SR_TC_CHANGE,
	/* Hard reset causes */
	__IECM_HR_FUNC_RESET, /* Hard reset when txrx timeout */
	__IECM_HR_CORE_RESET, /* when reset event is received on virtchannel */
	__IECM_HR_DRV_LOAD, /* Set on driver load for a clean HW */
	/* Generic bits to share a message */
	__IECM_DEL_QUEUES,
	__IECM_UP_REQUESTED, /* Set if open to be called explicitly by driver */
	/* Mailbox interrupt event */
	__IECM_MB_INTR_MODE,
	__IECM_MB_INTR_TRIGGER,
	/* Backwards compatibility */
	__IECM_NO_EXTENDED_CAPS,
	/* must be last */
	__IECM_FLAGS_NBITS,
};

struct iecm_netdev_priv {
	struct iecm_vport *vport;
};

struct iecm_reset_reg {
	u32 rstat;
	u32 rstat_m;
};

/* product specific register API */
struct iecm_reg_ops {
	void (*ctlq_reg_init)(struct iecm_ctlq_create_info *cq);
	void (*vportq_reg_init)(struct iecm_vport *vport);
	void (*intr_reg_init)(struct iecm_vport *vport);
	void (*mb_intr_reg_init)(struct iecm_adapter *adapter);
	void (*reset_reg_init)(struct iecm_reset_reg *reset_reg);
	void (*trigger_reset)(struct iecm_adapter *adapter,
			      enum iecm_flags trig_cause);
};

struct iecm_virtchnl_ops {
	int (*core_init)(struct iecm_adapter *adapter, int *vport_id);
	void (*vport_init)(struct iecm_vport *vport, int vport_id);
	int (*vport_queue_ids_init)(struct iecm_vport *vport);
	int (*get_caps)(struct iecm_adapter *adapter);
	int (*config_queues)(struct iecm_vport *vport);
	int (*enable_queues)(struct iecm_vport *vport);
	int (*disable_queues)(struct iecm_vport *vport);
	int (*irq_map_unmap)(struct iecm_vport *vport, bool map);
	int (*enable_vport)(struct iecm_vport *vport);
	int (*disable_vport)(struct iecm_vport *vport);
	int (*destroy_vport)(struct iecm_vport *vport);
	int (*get_ptype)(struct iecm_vport *vport);
	int (*get_set_rss_lut)(struct iecm_vport *vport, bool get);
	int (*get_set_rss_hash)(struct iecm_vport *vport, bool get);
	void (*adjust_qs)(struct iecm_vport *vport);
	int (*recv_mbx_msg)(struct iecm_adapter *adapter,
			    void *msg, int msg_size,
			    struct iecm_ctlq_msg *ctlq_msg, bool *work_done);
	bool (*is_cap_ena)(struct iecm_adapter *adapter, u64 flag);
};

struct iecm_dev_ops {
	void (*reg_ops_init)(struct iecm_adapter *adapter);
	void (*vc_ops_init)(struct iecm_adapter *adapter);
	void (*crc_enable)(u64 *td_cmd);
	struct iecm_reg_ops reg_ops;
	struct iecm_virtchnl_ops vc_ops;
};

/* vport specific data structure */

enum  iecm_vport_vc_state {
	IECM_VC_ENA_VPORT,
	IECM_VC_ENA_VPORT_ERR,
	IECM_VC_DIS_VPORT,
	IECM_VC_DIS_VPORT_ERR,
	IECM_VC_DESTROY_VPORT,
	IECM_VC_DESTROY_VPORT_ERR,
	IECM_VC_CONFIG_TXQ,
	IECM_VC_CONFIG_TXQ_ERR,
	IECM_VC_CONFIG_RXQ,
	IECM_VC_CONFIG_RXQ_ERR,
	IECM_VC_CONFIG_Q,
	IECM_VC_CONFIG_Q_ERR,
	IECM_VC_ENA_QUEUES,
	IECM_VC_ENA_QUEUES_ERR,
	IECM_VC_DIS_QUEUES,
	IECM_VC_DIS_QUEUES_ERR,
	IECM_VC_MAP_IRQ,
	IECM_VC_MAP_IRQ_ERR,
	IECM_VC_UNMAP_IRQ,
	IECM_VC_UNMAP_IRQ_ERR,
	IECM_VC_ADD_QUEUES,
	IECM_VC_ADD_QUEUES_ERR,
	IECM_VC_DEL_QUEUES,
	IECM_VC_DEL_QUEUES_ERR,
	IECM_VC_ALLOC_VECTORS,
	IECM_VC_ALLOC_VECTORS_ERR,
	IECM_VC_DEALLOC_VECTORS,
	IECM_VC_DEALLOC_VECTORS_ERR,
	IECM_VC_CREATE_VFS,
	IECM_VC_CREATE_VFS_ERR,
	IECM_VC_DESTROY_VFS,
	IECM_VC_DESTROY_VFS_ERR,
	IECM_VC_GET_RSS_HASH,
	IECM_VC_GET_RSS_HASH_ERR,
	IECM_VC_SET_RSS_HASH,
	IECM_VC_SET_RSS_HASH_ERR,
	IECM_VC_GET_RSS_LUT,
	IECM_VC_GET_RSS_LUT_ERR,
	IECM_VC_SET_RSS_LUT,
	IECM_VC_SET_RSS_LUT_ERR,
	IECM_VC_GET_RSS_KEY,
	IECM_VC_GET_RSS_KEY_ERR,
	IECM_VC_CONFIG_RSS_KEY,
	IECM_VC_CONFIG_RSS_KEY_ERR,
	IECM_VC_GET_STATS,
	IECM_VC_GET_STATS_ERR,
	IECM_VC_NBITS
};

enum iecm_vport_flags {
	__IECM_VPORT_SW_MARKER,
	__IECM_VPORT_FLAGS_NBITS,
};

struct iecm_vport {
	/* TX */
	unsigned int num_txq;
	unsigned int num_complq;
	/* It makes more sense for descriptor count to be part of only iecm
	 * queue structure. But when user changes the count via ethtool, driver
	 * has to store that value somewhere other than queue structure as the
	 * queues will be freed and allocated again.
	 */
	unsigned int txq_desc_count;
	unsigned int complq_desc_count;
	unsigned int compln_clean_budget;
	unsigned int num_txq_grp;
	struct iecm_txq_group *txq_grps;
	enum virtchnl_queue_model txq_model;
	/* Used only in hotpath to get to the right queue very fast */
	struct iecm_queue **txqs;
	wait_queue_head_t sw_marker_wq;
	DECLARE_BITMAP(flags, __IECM_VPORT_FLAGS_NBITS);

	/* RX */
	unsigned int num_rxq;
	unsigned int num_bufq;
	unsigned int rxq_desc_count;
	unsigned int bufq_desc_count;
	unsigned int num_rxq_grp;
	struct iecm_rxq_group *rxq_grps;
	enum virtchnl_queue_model rxq_model;
	struct iecm_rx_ptype_decoded rx_ptype_lkup[IECM_RX_MAX_PTYPE];

	struct iecm_adapter *adapter;
	struct net_device *netdev;
	u16 vport_type;
	u16 vport_id;
	u16 idx;		/* software index in adapter vports struct */
	struct rtnl_link_stats64 netstats;

	/* handler for hard interrupt */
	irqreturn_t (*irq_q_handler)(int irq, void *data);
	struct iecm_q_vector *q_vectors;	/* q vector array */
	u16 num_q_vectors;
	u16 q_vector_base;
	u16 max_mtu;
	u8 default_mac_addr[ETH_ALEN];
	u16 qset_handle;
	/* Duplicated in queue structure for performance reasons */
	enum iecm_rx_hsplit rx_hsplit_en;
};

/* User defined configuration values */
struct iecm_user_config_data {
	u32 num_req_tx_qs; /* user requested TX queues through ethtool */
	u32 num_req_rx_qs; /* user requested RX queues through ethtool */
	u32 num_req_txq_desc;
	u32 num_req_rxq_desc;
	void *req_qs_chunks;
};

struct iecm_rss_data {
	u64 rss_hash;
	u16 rss_key_size;
	u8 *rss_key;
	u16 rss_lut_size;
	u8 *rss_lut;
};

struct iecm_adapter {
	struct pci_dev *pdev;

	u32 tx_timeout_count;
	u32 msg_enable;
	enum iecm_state state;
	DECLARE_BITMAP(flags, __IECM_FLAGS_NBITS);
	struct mutex reset_lock; /* lock to protect reset flows */
	struct iecm_reset_reg reset_reg;
	struct iecm_hw hw;

	u16 num_req_msix;
	u16 num_msix_entries;
	struct msix_entry *msix_entries;
	struct virtchnl_alloc_vectors *req_vec_chunks;
	struct iecm_q_vector mb_vector;
	/* handler for hard interrupt for mailbox*/
	irqreturn_t (*irq_mb_handler)(int irq, void *data);

	/* vport structs */
	struct iecm_vport **vports;	/* vports created by the driver */
	u16 num_alloc_vport;
	u16 next_vport;		/* Next free slot in pf->vport[] - 0-based! */
	struct mutex sw_mutex;	/* lock to protect vport alloc flow */

	struct delayed_work init_task; /* delayed init task */
	struct workqueue_struct *init_wq;
	u32 mb_wait_count;
	struct delayed_work serv_task; /* delayed service task */
	struct workqueue_struct *serv_wq;
	struct delayed_work stats_task; /* delayed statistics task */
	struct workqueue_struct *stats_wq;
	struct delayed_work vc_event_task; /* delayed virtchannel event task */
	struct workqueue_struct *vc_event_wq;
	/* Store the resources data received from control plane */
	void **vport_params_reqd;
	void **vport_params_recvd;
	/* User set parameters */
	struct iecm_user_config_data config_data;
	void *caps;
	wait_queue_head_t vchnl_wq;
	DECLARE_BITMAP(vc_state, IECM_VC_NBITS);
	struct mutex vc_msg_lock;	/* lock to protect vc_msg flow */
	char vc_msg[IECM_DFLT_MBX_BUF_SIZE];
	struct iecm_rss_data rss_data;
	struct iecm_dev_ops dev_ops;
	enum virtchnl_link_speed link_speed;
	bool link_up;
	int debug_msk; /* netif messaging level */
};

/**
 * iecm_is_queue_model_split - check if queue model is split
 * @q_model: queue model single or split
 *
 * Returns true if queue model is split else false
 */
static inline int iecm_is_queue_model_split(enum virtchnl_queue_model q_model)
{
	return (q_model == VIRTCHNL_QUEUE_MODEL_SPLIT);
}

/**
 * iecm_is_cap_ena - Determine if HW capability is supported
 * @adapter: private data struct
 * @flag: Feature flag to check
 */
static inline bool iecm_is_cap_ena(struct iecm_adapter *adapter, u64 flag)
{
	return adapter->dev_ops.vc_ops.is_cap_ena(adapter, flag);
}

/**
 * iecm_is_feature_ena - Determine if a particular feature is enabled
 * @vport: vport to check
 * @feature: netdev flag to check
 *
 * Returns true or false if a particular feature is enabled.
 */
static inline bool iecm_is_feature_ena(struct iecm_vport *vport,
				       netdev_features_t feature)
{
	return vport->netdev->features & feature;
}

/**
 * iecm_rx_offset - Return expected offset into page to access data
 * @rx_q: queue we are requesting offset of
 *
 * Returns the offset value for queue into the data buffer.
 */
static inline unsigned int
iecm_rx_offset(struct iecm_queue __maybe_unused *rx_q)
{
	return 0;
}

int iecm_probe(struct pci_dev *pdev,
	       const struct pci_device_id __always_unused *ent,
	       struct iecm_adapter *adapter);
void iecm_remove(struct pci_dev *pdev);
void iecm_shutdown(struct pci_dev *pdev);
void iecm_vport_adjust_qs(struct iecm_vport *vport);
int iecm_ctlq_reg_init(struct iecm_ctlq_create_info *cq, int num_q);
int iecm_init_dflt_mbx(struct iecm_adapter *adapter);
void iecm_deinit_dflt_mbx(struct iecm_adapter *adapter);
void iecm_vc_ops_init(struct iecm_adapter *adapter);
int iecm_vc_core_init(struct iecm_adapter *adapter, int *vport_id);
int iecm_wait_for_event(struct iecm_adapter *adapter,
			enum iecm_vport_vc_state state,
			enum iecm_vport_vc_state err_check);
int iecm_send_get_caps_msg(struct iecm_adapter *adapter);
int iecm_send_delete_queues_msg(struct iecm_vport *vport);
int iecm_send_add_queues_msg(struct iecm_vport *vport, u16 num_tx_q,
			     u16 num_complq, u16 num_rx_q, u16 num_rx_bufq);
int iecm_initiate_soft_reset(struct iecm_vport *vport,
			     enum iecm_flags reset_cause);
int iecm_send_config_tx_queues_msg(struct iecm_vport *vport);
int iecm_send_config_rx_queues_msg(struct iecm_vport *vport);
int iecm_send_enable_vport_msg(struct iecm_vport *vport);
int iecm_send_disable_vport_msg(struct iecm_vport *vport);
int iecm_send_destroy_vport_msg(struct iecm_vport *vport);
int iecm_send_get_rx_ptype_msg(struct iecm_vport *vport);
int iecm_send_get_set_rss_key_msg(struct iecm_vport *vport, bool get);
int iecm_send_get_set_rss_lut_msg(struct iecm_vport *vport, bool get);
int iecm_send_get_set_rss_hash_msg(struct iecm_vport *vport, bool get);
int iecm_send_dealloc_vectors_msg(struct iecm_adapter *adapter);
int iecm_send_alloc_vectors_msg(struct iecm_adapter *adapter, u16 num_vectors);
int iecm_send_enable_strip_vlan_msg(struct iecm_vport *vport);
int iecm_send_disable_strip_vlan_msg(struct iecm_vport *vport);
int iecm_vport_params_buf_alloc(struct iecm_adapter *adapter);
void iecm_vport_params_buf_rel(struct iecm_adapter *adapter);
struct iecm_vport *iecm_netdev_to_vport(struct net_device *netdev);
struct iecm_adapter *iecm_netdev_to_adapter(struct net_device *netdev);
int iecm_send_get_stats_msg(struct iecm_vport *vport);
int iecm_vport_get_vec_ids(u16 *vecids, int num_vecids,
			   struct virtchnl_vector_chunks *chunks);
int iecm_recv_mb_msg(struct iecm_adapter *adapter, enum virtchnl_ops op,
		     void *msg, int msg_size);
int iecm_send_mb_msg(struct iecm_adapter *adapter, enum virtchnl_ops op,
		     u16 msg_size, u8 *msg);
void iecm_set_ethtool_ops(struct net_device *netdev);
void iecm_vport_set_hsplit(struct iecm_vport *vport, struct bpf_prog *prog);
void iecm_vport_intr_rel(struct iecm_vport *vport);
int iecm_vport_intr_alloc(struct iecm_vport *vport);
#endif /* !_IECM_H_ */
