/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2016-2019 Intel Corporation. */
#ifndef _DATAPATH_GSWIP_H_
#define _DATAPATH_GSWIP_H_

#include <linux/device.h>
#include <linux/bits.h>

#define BR_PORT_MAP_NUM		16
#define PMAC_BSL_THRES_NUM	3
#define PMAC_HEADER_NUM		8

enum gswip_cpu_parser_hdr_cfg {
	GSWIP_CPU_PARSER_NIL,
	GSWIP_CPU_PARSER_FLAGS,
	GSWIP_CPU_PARSER_OFFSETS_FLAGS,
	GSWIP_CPU_PARSER_RESERVED,
};

struct gswip_cpu_port_cfg {
	u8 lpid;
	bool is_cpu_port;
	bool spec_tag_ig;
	bool spec_tag_eg;
	bool fcs_chk;
	bool fcs_generate;
	enum gswip_cpu_parser_hdr_cfg no_mpe_parser_cfg;
	enum gswip_cpu_parser_hdr_cfg mpe1_parser_cfg;
	enum gswip_cpu_parser_hdr_cfg mpe2_parser_cfg;
	enum gswip_cpu_parser_hdr_cfg mpe3_parser_cfg;
	bool ts_rm_ptp_pkt;
	bool ts_rm_non_ptp_pkt;
};

enum gswip_pmac_short_frm_chk {
	GSWIP_PMAC_SHORT_LEN_DIS,
	GSWIP_PMAC_SHORT_LEN_ENA_UNTAG,
	GSWIP_PMAC_SHORT_LEN_ENA_TAG,
	GSWIP_PMAC_SHORT_LEN_RESERVED,
};

enum gswip_pmac_proc_flags_eg_cfg {
	GSWIP_PMAC_PROC_FLAGS_NONE,
	GSWIP_PMAC_PROC_FLAGS_TC,
	GSWIP_PMAC_PROC_FLAGS_FLAG,
	GSWIP_PMAC_PROC_FLAGS_MIX,
};

struct gswip_pmac_glb_cfg {
	u8 pmac_id;
	bool apad_en;
	bool pad_en;
	bool vpad_en;
	bool svpad_en;
	bool rx_fcs_dis;
	bool tx_fcs_dis;
	bool ip_trans_chk_reg_dis;
	bool ip_trans_chk_ver_dis;
	bool jumbo_en;
	u16 max_jumbo_len;
	u16 jumbo_thresh_len;
	bool long_frm_chk_dis;
	enum gswip_pmac_short_frm_chk short_frm_chk_type;
	bool proc_flags_eg_cfg_en;
	enum gswip_pmac_proc_flags_eg_cfg proc_flags_eg_cfg;
	u16 bsl_thresh[PMAC_BSL_THRES_NUM];
};

struct gswip_pmac_bp_map {
	u8 pmac_id;
	u8 tx_dma_chan_id;
	u32 tx_q_mask;
	u32 rx_port_mask;
};

enum gswip_pmac_ig_cfg_src {
	GSWIP_PMAC_IG_CFG_SRC_DMA_DESC,
	GSWIP_PMAC_IG_CFG_SRC_DEF_PMAC,
	GSWIP_PMAC_IG_CFG_SRC_PMAC,
};

struct gswip_pmac_ig_cfg {
	u8 pmac_id;
	u8 tx_dma_chan_id;
	bool err_pkt_disc;
	bool class_def;
	bool class_en;
	enum gswip_pmac_ig_cfg_src sub_id;
	bool src_port_id_def;
	bool pmac_present;
	u8 def_pmac_hdr[PMAC_HEADER_NUM];
};

struct gswip_pmac_eg_cfg {
	u8 pmac_id;
	u8 dst_port_id;
	u8 tc;
	bool mpe1;
	bool mpe2;
	bool decrypt;
	bool encrypt;
	u8 flow_id_msb;
	bool process_sel;
	u8 rx_dma_chan_id;
	bool rm_l2_hdr;
	u8 num_byte_rm;
	bool fcs_en;
	bool pmac_en;
	bool redir_en;
	bool bsl_seg_disable;
	u8 bsl_tc;
	bool resv_dw1_en;
	u8 resv_dw1;
	bool resv_1dw0_en;
	u8 resv_1dw0;
	bool resv_2dw0_en;
	u8 resv_2dw0;
	bool tc_en;
};

struct gswip_lpid2gpid {
	u16 lpid;
	u16 first_gpid;
	u16 num_gpid;
	u8 valid_bits;
};

struct gswip_gpid2lpid {
	u16 gpid;
	u16 lpid;
	u8 subif_grp_field;
	bool subif_grp_field_ovr;
};

enum gswip_ctp_port_config_mask {
	GSWIP_CTP_PORT_CONFIG_MASK_BRIDGE_PORT_ID	= BIT(0),
	GSWIP_CTP_PORT_CONFIG_MASK_FORCE_TRAFFIC_CLASS	= BIT(1),
	GSWIP_CTP_PORT_CONFIG_MASK_INGRESS_VLAN		= BIT(2),
	GSWIP_CTP_PORT_CONFIG_MASK_INGRESS_VLAN_IGMP	= BIT(3),
	GSWIP_CTP_PORT_CONFIG_MASK_EGRESS_VLAN		= BIT(4),
	GSWIP_CTP_PORT_CONFIG_MASK_EGRESS_VLAN_IGMP	= BIT(5),
	GSWIP_CTP_PORT_CONFIG_MASK_INRESS_NTO1_VLAN	= BIT(6),
	GSWIP_CTP_PORT_CONFIG_MASK_EGRESS_NTO1_VLAN	= BIT(7),
	GSWIP_CTP_PORT_CONFIG_INGRESS_MARKING		= BIT(8),
	GSWIP_CTP_PORT_CONFIG_EGRESS_MARKING		= BIT(9),
	GSWIP_CTP_PORT_CONFIG_EGRESS_MARKING_OVERRIDE	= BIT(10),
	GSWIP_CTP_PORT_CONFIG_EGRESS_REMARKING		= BIT(11),
	GSWIP_CTP_PORT_CONFIG_INGRESS_METER		= BIT(12),
	GSWIP_CTP_PORT_CONFIG_EGRESS_METER		= BIT(13),
	GSWIP_CTP_PORT_CONFIG_BRIDGING_BYPASS		= BIT(14),
	GSWIP_CTP_PORT_CONFIG_FLOW_ENTRY		= BIT(15),
	GSWIP_CTP_PORT_CONFIG_LOOPBACK_AND_MIRROR	= BIT(16),
	GSWIP_CTP_PORT_CONFIG_MASK_FORCE		= BIT(31),
};

struct gswip_ctp_port_cfg {
	u8 lpid;
	u16 subif_id_grp;
	u16 br_pid;
	enum gswip_ctp_port_config_mask mask;
	bool br_bypass;
};

enum gswip_lport_mode {
	GSWIP_LPORT_8BIT_WLAN,
	GSWIP_LPORT_9BIT_WLAN,
	GSWIP_LPORT_GPON,
	GSWIP_LPORT_EPON,
	GSWIP_LPORT_GINT,
	GSWIP_LPORT_OTHER = 0xFF,
};

struct gswip_ctp_port_info {
	u8 lpid;
	u16 first_pid;
	u16 num_port;
	enum gswip_lport_mode mode;
	u16 br_pid;
};

enum gswip_br_port_cfg_mask {
	GSWIP_BR_PORT_CFG_MASK_BR_ID			= BIT(0),
	GSWIP_BR_PORT_CFG_MASK_IG_VLAN			= BIT(1),
	GSWIP_BR_PORT_CFG_MASK_EG_VLAN			= BIT(2),
	GSWIP_BR_PORT_CFG_MASK_IG_MARKING		= BIT(3),
	GSWIP_BR_PORT_CFG_MASK_EG_REMARKING		= BIT(4),
	GSWIP_BR_PORT_CFG_MASK_IG_METER			= BIT(5),
	GSWIP_BR_PORT_CFG_MASK_EG_SUB_METER		= BIT(6),
	GSWIP_BR_PORT_CFG_MASK_EG_CTP_MAPPING		= BIT(7),
	GSWIP_BR_PORT_CFG_MASK_BR_PORT_MAP		= BIT(8),
	GSWIP_BR_PORT_CFG_MASK_MCAST_DST_IP_LOOKUP	= BIT(9),
	GSWIP_BR_PORT_CFG_MASK_MCAST_SRC_IP_LOOKUP	= BIT(10),
	GSWIP_BR_PORT_CFG_MASK_MCAST_DST_MAC_LOOKUP	= BIT(11),
	GSWIP_BR_PORT_CFG_MASK_MCAST_SRC_MAC_LEARNING	= BIT(12),
	GSWIP_BR_PORT_CFG_MASK_MAC_SPOOFING		= BIT(13),
	GSWIP_BR_PORT_CFG_MASK_PORT_LOCK		= BIT(14),
	GSWIP_BR_PORT_CFG_MASK_MAC_LEARNING_LIMIT	= BIT(15),
	GSWIP_BR_PORT_CFG_MASK_MAC_LEARNED_COUNT	= BIT(16),
	GSWIP_BR_PORT_CFG_MASK_IG_VLAN_FILTER		= BIT(17),
	GSWIP_BR_PORT_CFG_MASK_EG_VLAN_FILTER1		= BIT(18),
	GSWIP_BR_PORT_CFG_MASK_EG_VLAN_FILTER2		= BIT(19),
	GSWIP_BR_PORT_CFG_MASK_FORCE			= BIT(31),
};

enum gswip_pmapper_mapping_mode {
	GSWIP_PMAPPER_MAPPING_PCP,
	GSWIP_PMAPPER_MAPPING_DSCP,
};

enum gswip_br_port_eg_meter {
	GSWIP_BR_PORT_EG_METER_BROADCAST,
	GSWIP_BR_PORT_EG_METER_MULTICAST,
	GSWIP_BR_PORT_EG_METER_UNKNOWN_MCAST_IP,
	GSWIP_BR_PORT_EG_METER_UNKNOWN_MCAST_NON_IP,
	GSWIP_BR_PORT_EG_METER_UNKNOWN_UCAST,
	GSWIP_BR_PORT_EG_METER_OTHERS,
	GSWIP_BR_PORT_EG_METER_MAX,
};

struct gswip_br_port_alloc {
	u8 br_id;
	u8 br_pid;
};

enum gswip_br_cfg_mask {
	GSWIP_BR_CFG_MASK_MAC_LEARNING_LIMIT	= BIT(0),
	GSWIP_BR_CFG_MASK_MAC_LEARNED_COUNT	= BIT(1),
	GSWIP_BR_CFG_MASK_MAC_DISCARD_COUNT	= BIT(2),
	GSWIP_BR_CFG_MASK_SUB_METER		= BIT(3),
	GSWIP_BR_CFG_MASK_FORWARDING_MODE	= BIT(4),
	GSWIP_BR_CFG_MASK_FORCE			= BIT(31),
};

enum gswip_br_fwd_mode {
	GSWIP_BR_FWD_FLOOD,
	GSWIP_BR_FWD_DISCARD,
	GSWIP_BR_FWD_CPU,
};

struct gswip_br_alloc {
	u8 br_id;
};

struct gswip_br_cfg {
	u8 br_id;
	enum gswip_br_cfg_mask mask;
	bool mac_lrn_limit_en;
	u16 mac_lrn_limit;
	u16 mac_lrn_count;
	u32 lrn_disc_event;
	enum gswip_br_fwd_mode fwd_bcast;
	enum gswip_br_fwd_mode fwd_unk_mcast_ip;
	enum gswip_br_fwd_mode fwd_unk_mcast_non_ip;
	enum gswip_br_fwd_mode fwd_unk_ucast;
};

struct gswip_qos_wred_q_cfg {
	u8 qid;
	u16 red_min;
	u16 red_max;
	u16 yellow_min;
	u16 yellow_max;
	u16 green_min;
	u16 green_max;
};

struct gswip_qos_wred_port_cfg {
	u8 lpid;
	u16 red_min;
	u16 red_max;
	u16 yellow_min;
	u16 yellow_max;
	u16 green_min;
	u16 green_max;
};

enum gswip_qos_q_map_mode {
	GSWIP_QOS_QMAP_SINGLE_MD,
	GSWIP_QOS_QMAP_SUBIFID_MD,
};

struct gswip_qos_q_port {
	u8 lpid;
	bool extration_en;
	enum gswip_qos_q_map_mode q_map_mode;
	u8 tc_id;
	u8 qid;
	bool egress;
	u8 redir_port_id;
	bool en_ig_pce_bypass;
	bool resv_port_mode;
};

struct gswip_register {
	u16 offset;
	u16 data;
};

enum {
	SPEED_LMAC_10M,
	SPEED_LMAC_100M,
	SPEED_LMAC_200M,
	SPEED_LMAC_1G,
	SPEED_XGMAC_10M,
	SPEED_XGMAC_100M,
	SPEED_XGMAC_1G,
	SPEED_XGMII_25G,
	SPEED_XGMAC_5G,
	SPEED_XGMAC_10G,
	SPEED_GMII_25G,
	SPEED_MAC_AUTO,
};

enum gsw_portspeed {
	/* 10 Mbit/s */
	GSW_PORT_SPEED_10 = 10,

	/* 100 Mbit/s */
	GSW_PORT_SPEED_100 = 100,

	/* 200 Mbit/s */
	GSW_PORT_SPEED_200 = 200,

	/* 1000 Mbit/s */
	GSW_PORT_SPEED_1000 = 1000,

	/* 2.5 Gbit/s */
	GSW_PORT_SPEED_25000 = 25000,

	/* 10 Gbit/s */
	GSW_PORT_SPEED_100000 = 100000,
};

enum gsw_portlink_status {
	LINK_AUTO,
	LINK_UP,
	LINK_DOWN,
};

enum gsw_flow_control_modes {
	FC_AUTO,
	FC_RX,
	FC_TX,
	FC_RXTX,
	FC_DIS,
	FC_INVALID,
};

/* Ethernet port interface mode. */
enum gsw_mii_mode {
	/* Normal PHY interface (twisted pair), use internal MII Interface */
	GSW_PORT_HW_MII,

	/* Reduced MII interface in normal mode */
	GSW_PORT_HW_RMII,

	/* GMII or MII, depending upon the speed */
	GSW_PORT_HW_GMII,

	/* RGMII mode */
	GSW_PORT_HW_RGMII,

	/* XGMII mode */
	GSW_PORT_HW_XGMII,
};

struct core_common_ops {
	int (*enable)(struct device *dev, bool enable);
	int (*cpu_port_cfg_get)(struct device *dev,
				struct gswip_cpu_port_cfg *cpu);
	int (*reg_get)(struct device *dev, struct gswip_register *param);
	int (*reg_set)(struct device *dev, struct gswip_register *param);
};

struct core_pmac_ops {
	int (*gbl_cfg_set)(struct device *dev, struct gswip_pmac_glb_cfg *pmac);
	int (*bp_map_get)(struct device *dev, struct gswip_pmac_bp_map *bp);
	int (*ig_cfg_set)(struct device *dev, struct gswip_pmac_ig_cfg *ig);
	int (*eg_cfg_set)(struct device *dev, struct gswip_pmac_eg_cfg *eg);
};

struct core_gpid_ops {
	int (*lpid2gpid_set)(struct device *dev,
			     struct gswip_lpid2gpid *lp2gp);
	int (*lpid2gpid_get)(struct device *dev,
			     struct gswip_lpid2gpid *lp2gp);
	int (*gpid2lpid_set)(struct device *dev,
			     struct gswip_gpid2lpid *gp2lp);
	int (*gpid2lpid_get)(struct device *dev,
			     struct gswip_gpid2lpid *gp2lp);
};

struct core_ctp_ops {
	int (*alloc)(struct device *dev, struct gswip_ctp_port_info *ctp);
	int (*free)(struct device *dev, u8 lpid);
};

struct core_br_port_ops {
	int (*alloc)(struct device *dev, struct gswip_br_port_alloc *bp);
	int (*free)(struct device *dev, struct gswip_br_port_alloc *bp);
};

struct core_br_ops {
	int (*alloc)(struct device *dev, struct gswip_br_alloc *br);
	int (*free)(struct device *dev, struct gswip_br_alloc *br);
};

struct core_qos_ops {
	int (*q_port_set)(struct device *dev, struct gswip_qos_q_port *qport);
	int (*wred_q_cfg_set)(struct device *dev,
			      struct gswip_qos_wred_q_cfg *wredq);
	int (*wred_port_cfg_set)(struct device *dev,
				 struct gswip_qos_wred_port_cfg *wredp);
};

struct gsw_mac_ops {
	/* Initialize MAC */
	int (*init)(struct device *dev);

	int (*set_macaddr)(struct device *dev, u8 *mac_addr);
	int (*get_link_sts)(struct device *dev);

	int (*get_duplex)(struct device *dev);

	int (*set_speed)(struct device *dev, u8 speed);
	int (*get_speed)(struct device *dev);

	u32 (*get_mtu)(struct device *dev);
	int (*set_mtu)(struct device *dev, u32 mtu);

	u32 (*get_flowctrl)(struct device *dev);
	int (*set_flowctrl)(struct device *dev, u32 val);

	int (*get_pfsa)(struct device *dev, u8 *addr, u32 *mode);
	int (*set_pfsa)(struct device *dev, u8 *mac_addr, u32 macif);

	int (*get_mii_if)(struct device *dev);
	int (*set_mii_if)(struct device *dev, u32 mii_mode);

	u32 (*get_lpi)(struct device *dev);

	int (*get_fcsgen)(struct device *dev);
};

struct gsw_adap_ops {
	int (*sw_core_enable)(struct device *dev, u32 val);
};
#endif
