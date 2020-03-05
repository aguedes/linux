/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2016-2019 Intel Corporation. */
#ifndef _GSW_TBL_RW_H_
#define _GSW_TBL_RW_H_

#define PCE_TBL_KEY_NUM		34
#define PCE_TBL_VAL_NUM		31
#define PCE_TBL_MASK_NUM	4
#define PCE_MAC_LIMIT_NUM	255

/* PCE queue mapping table */
#define PCE_Q_MAP_LOCAL_EXTRACT			BIT(8)
#define PCE_Q_MAP_IG_PORT_MODE			BIT(9)
#define PCE_Q_MAP_EG_PID			GENMASK(7, 4)

/* PCE ingress CTP port configuration table */
#define PCE_IGCTP_VAL4_BYPASS_BR		BIT(15)

/* PCE ingress bridge port configuration table */
#define PCE_IGBGP_VAL10_PORT_MAP_0		10
#define PCE_IGBGP_VAL4_LRN_LIMIT		GENMASK(7, 0)
#define PCE_IGBGP_VAL4_BR_ID			GENMASK(13, 8)

/* PCE EGBGP table */
#define PCE_EGBGP_VAL4_DST_SUBIF_ID_GRP		GENMASK(7, 0)
#define PCE_EGBGP_VAL4_DST_LPID			GENMASK(11, 8)
#define PCE_EGBGP_VAL4_PMAPPER			BIT(14)

/* PCE_BRGCFG table */
#define PCE_BRGCFG_VAL1_BCAST_FW_MODE		GENMASK(1, 0)
#define PCE_BRGCFG_VAL1_UCAST_FW_MODE		GENMASK(3, 2)
#define PCE_BRGCFG_VAL1_MCAST_NIP_FW_MODE	GENMASK(5, 4)
#define PCE_BRGCFG_VAL1_MCAST_IP_FW_MODE	GENMASK(7, 6)
#define PCE_BRGCFG_VAL3_LRN_DISC_CNT		GENMASK(31, 16)

/* BM_PQM_THRES table */
#define BM_PQM_THRES_Q_NUM			GENMASK(7, 3)

/* BM_Q_MAP table */
#define BM_Q_MAP_VAL4_REDIR_PID			GENMASK(2, 0)
#define BM_Q_MAP_VAL4_REDIR_PID_MSB		BIT(3)
#define BM_Q_MAP_VAL4_REDIR_PID_BIT4		BIT(4)

enum pce_tbl_id {
	PCE_TFLOW	= 0x0f,
	PCE_Q_MAP	= 0x11,
	PCE_IG_CTP_CFG	= 0x12,
	PCE_EG_CTP_CFG	= 0x13,
	PCE_IG_BRP_CFG	= 0x14,
	PCE_EG_BRP_CFG	= 0x15,
	PCE_BR_CFG	= 0x19,
	PCE_PMAP	= 0x1f,
};

#define PCE_TBL_KEY_NUM		34
#define PCE_TBL_VAL_NUM		31
#define PCE_TBL_MASK_NUM	4

/* PCE programming table */
struct pce_tbl_prog {
	enum pce_tbl_id id;
	u16 val[PCE_TBL_VAL_NUM];
	u16 key[PCE_TBL_KEY_NUM];
	u16 mask[PCE_TBL_MASK_NUM];
	u16 addr;
};

/* BM programming table */
enum bm_tbl_id {
	BM_CTP_RX_RMON		= 0x00,
	BM_CTP_TX_RMON		= 0x01,
	BM_BR_RX_RMON		= 0x02,
	BM_BR_TX_RMON		= 0x03,
	BM_CTP_PCE_BYPASS_RMON	= 0x04,
	BM_TFLOW_RX_RMON	= 0x05,
	BM_TFLOW_TX_RMON	= 0x06,
	BM_WFQ_PARAM		= 0x07,
	BM_PQM_THRES		= 0x09,
	BM_Q_MAP		= 0x0e,
	BM_PMAC_CNTR		= 0x1c
};

#define BM_RAM_VAL_MAX		10

struct rmon_cntr_tbl {
	u16 ctr_offset : 6;
	u16 port_offset : 10;
};

struct qmap_tbl {
	u16 qid : 6;
	u16 reserved : 10;
};

struct pmac_cntr_tbl {
	u16 addr : 5;
	u16 ctr_offset : 3;
	u16 pmac_id : 3;
	u16 reserved : 1;
	u16 ctr_offset_hdr : 1;
	u16 reserved1 : 3;
};

struct bm_tbl_prog {
	enum bm_tbl_id id;
	union {
		struct rmon_cntr_tbl rmon_cntr;
		struct qmap_tbl qmap;
		struct pmac_cntr_tbl pmac_cntr;
		u16 addr;
	};
	u16 val[BM_RAM_VAL_MAX];
	u32 num_val;
};

/* PMAC programming table */
enum pmac_tbl_id {
	PMAC_BP_MAP,
	PMAC_IG_CFG,
	PMAC_EG_CFG,
};

#define PMAC_BP_MAP_TBL_VAL_NUM		3
#define PMAC_IG_CFG_TBL_VAL_NUM		5
#define PMAC_EG_CFG_TBL_VAL_NUM		3
#define PMAC_TBL_VAL_MAX		11

/* PMAC_BP_MAP table */
/* Table Control */
#define PMAC_BPMAP_TX_DMA_CH		GENMASK(4, 0)
/* PMAC_TBL_VAL_2 */
#define PMAC_BPMAP_TX_Q_UPPER		GENMASK(31, 16)

/* PMAC_IG_CFG table */
/* Table Control */
#define PMAC_IGCFG_TX_DMA_CH		GENMASK(4, 0)
/* PMAC_TBL_VAL_2 */
#define PMAC_IGCFG_HDR_ID		GENMASK(7, 4)
/* PMAC_TBL_VAL_4 */
#define PMAC_IGCFG_VAL4_PMAC_FLAG	BIT(0)
#define PMAC_IGCFG_VAL4_SPPID_MODE	BIT(1)
#define PMAC_IGCFG_VAL4_SUBID_MODE	BIT(2)
#define PMAC_IGCFG_VAL4_CLASSEN_MODE	BIT(3)
#define PMAC_IGCFG_VAL4_CLASS_MODE	BIT(5)
#define PMAC_IGCFG_VAL4_ERR_DP		BIT(7)

/* PMAC_EG_CFG table */
/* Table Control */
#define PMAC_EGCFG_DST_PORT_ID		GENMASK(3, 0)
#define PMAC_EGCFG_MPE1			BIT(4)
#define PMAC_EGCFG_MPE2			BIT(5)
#define PMAC_EGCFG_ECRYPT		BIT(6)
#define PMAC_EGCFG_DECRYPT		BIT(7)
#define PMAC_EGCFG_TC_4BITS		GENMASK(7, 4)
#define PMAC_EGCFG_TC_2BITS		GENMASK(7, 6)
#define PMAC_EGCFG_FLOW_ID_MSB		GENMASK(9, 8)
/* PMAC_TBL_VAL_0 */
#define PMAC_EGCFG_VAL0_RES_2BITS	GENMASK(1, 0)
#define PMAC_EGCFG_VAL0_RES_3BITS	GENMASK(4, 2)
#define PMAC_EGCFG_VAL0_REDIR		BIT(4)
#define PMAC_EGCFG_VAL0_BSL		GENMASK(7, 5)
/* PMAC_TBL_VAL_1 */
#define PMAC_EGCFG_VAL1_RX_DMA_CH	GENMASK(3, 0)
/* PMAC_TBL_VAL_2 */
#define PMAC_EGCFG_VAL2_PMAC_FLAG	BIT(0)
#define PMAC_EGCFG_VAL2_FCS_MODE	BIT(1)
#define PMAC_EGCFG_VAL2_L2HD_RM_MODE	BIT(2)
#define PMAC_EGCFG_VAL2_L2HD_RM		GENMASK(15, 8)

struct pmac_tbl_prog {
	u16 pmac_id;
	enum pmac_tbl_id id;
	u16 val[PMAC_TBL_VAL_MAX];
	u8 num_val;
	u16 addr;
};

struct gswip_core_priv;

int gswip_pce_table_init(void);
int gswip_pce_table_write(struct gswip_core_priv *priv,
			  struct pce_tbl_prog *pce_tbl);
int gswip_pce_table_read(struct gswip_core_priv *priv,
			 struct pce_tbl_prog *pce_tbl);
int gswip_bm_table_read(struct gswip_core_priv *priv,
			struct bm_tbl_prog *bm_tbl);
int gswip_bm_table_write(struct gswip_core_priv *priv,
			 struct bm_tbl_prog *bm_tbl);
int gswip_pmac_table_read(struct gswip_core_priv *priv,
			  struct pmac_tbl_prog *pmac_tbl);
int gswip_pmac_table_write(struct gswip_core_priv *priv,
			   struct pmac_tbl_prog *pmac_tbl);

#endif

