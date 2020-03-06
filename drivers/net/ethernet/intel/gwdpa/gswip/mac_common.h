/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2016-2019 Intel Corporation. */

#ifndef _MAC_COMMON_H
#define _MAC_COMMON_H

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include "gswip.h"
#include "gswip_reg.h"
#include "gswip_dev.h"

#define LGM_MAX_MTU	10000
#define LPITX_EN	1
#define SPEED_LSB	GENMASK(1, 0)
#define SPEED_MSB	BIT(2)
#define MDIO_CLAUSE22	1
#define MDIO_CLAUSE45	0

/* MAC Index */
enum mac_index {
	PMAC0 = 0,
	PMAC1,
	MAC2,
	MAC3,
	MAC4,
	MAC5,
	MAC6,
	MAC7,
	MAC8,
	MAC9,
	MAC10,
	PMAC2,
	MAC_LAST,
};

enum packet_flilter_mode {
	PROMISC = 0,
	PASS_ALL_MULTICAST,
	PKT_FR_DEF,
};

enum mii_interface {
	LMAC_MII = 0,
	LMAC_GMII,
	XGMAC_GMII,
	XGMAC_XGMII,
};

enum speed_interface {
	SPEED_10M = 0,
	SPEED_100M,
	SPEED_1G,
	SPEED_10G,
	SPEED_2G5,
	SPEED_5G,
	SPEED_FLEX,
	SPEED_AUTO
};

enum gsw_fcon {
	FCONRX = 0,
	FCONTX,
	FDUP,
};

enum xgmac_fcon_on_off {
	XGMAC_FC_EN = 0,
	XGMAC_FC_DIS,
};

enum gsw_portduplex_mode {
	GSW_DUPLEX_AUTO = 0,
	GSW_DUPLEX_HALF,
	GSW_DUPLEX_FULL,
};

struct xgmac_hw_features {
	/* HW version */
	u32 version;

	/* HW Feature0 Register: core */
	u32 gmii;	/* 1000 Mbps support */
	u32 vlhash;	/* VLAN Hash Filter */
	u32 sma;	/* SMA(MDIO) Interface */
	u32 rwk;	/* PMT remote wake-up packet */
	u32 mgk;	/* PMT magic packet */
	u32 mmc;	/* RMON module */
	u32 aoe;	/* ARP Offload */
	u32 ts;		/* IEEE 1588-2008 Advanced Timestamp */
	u32 eee;	/* Energy Efficient Ethernet */
	u32 tx_coe;	/* Tx Checksum Offload */
	u32 rx_coe;	/* Rx Checksum Offload */
	u32 addn_mac;	/* Additional MAC Addresses */
	u32 ts_src;	/* Timestamp Source */
	u32 sa_vlan_ins;/* Source Address or VLAN Insertion */
	u32 vxn;	/* VxLAN/NVGRE Support */
	u32 ediffc;	/* Different Descriptor Cache */
	u32 edma;	/* Enhanced DMA */

	/* HW Feature1 Register: DMA and MTL */
	u32 rx_fifo_size;	/* MTL Receive FIFO Size */
	u32 tx_fifo_size;	/* MTL Transmit FIFO Size */
	u32 osten;		/* One-Step Timestamping Enable */
	u32 ptoen;		/* PTP Offload Enable */
	u32 adv_ts_hi;		/* Advance Timestamping High Word */
	u32 dma_width;		/* DMA width */
	u32 dcb;		/* DCB Feature */
	u32 sph;		/* Split Header Feature */
	u32 tso;		/* TCP Segmentation Offload */
	u32 dma_debug;		/* DMA Debug Registers */
	u32 rss;		/* Receive Side Scaling */
	u32 tc_cnt;		/* Number of Traffic Classes */
	u32 hash_table_size;	/* Hash Table Size */
	u32 l3l4_filter_num;	/* Number of L3-L4 Filters */

	/* HW Feature2 Register: Channels(DMA) and Queues(MTL) */
	u32 rx_q_cnt;		/* Number of MTL Receive Queues */
	u32 tx_q_cnt;		/* Number of MTL Transmit Queues */
	u32 rx_ch_cnt;		/* Number of DMA Receive Channels */
	u32 tx_ch_cnt;		/* Number of DMA Transmit Channels */
	u32 pps_out_num;	/* Number of PPS outputs */
	u32 aux_snap_num;	/* Number of Aux snapshot inputs */
};

struct gswip_mac {
	void __iomem *sw;	/* adaption layer */
	void __iomem *lmac;	/* legacy mac */

	/* XGMAC registers for indirect accessing */
	u32 xgmac_ctrl_reg;
	u32 xgmac_data0_reg;
	u32 xgmac_data1_reg;

	u32 sw_irq;
	struct clk *ptp_clk;
	struct clk *sw_clk;

	struct device *dev;
	struct device *parent;

	spinlock_t mac_lock;	/* MAC spin lock*/
	spinlock_t irw_lock;	/* lock for Indirect read/write */
	spinlock_t sw_lock;	/* adaption lock */

	/* Phy status */
	u32 phy_speed;
	const char *phy_mode;

	u32 ver;
	/* Index to point XGMAC 2/3/4/.. */
	u32 mac_idx;
	u32 mac_max;
	u32 ptp_clk_rate;

	struct xgmac_hw_features hw_feat;
	const struct gsw_mac_ops *mac_ops;

	const struct gsw_adap_ops *adap_ops;
	u32 core_en_cnt;
	struct mii_bus *mii;

	u8 mac_addr[6];
	u32 mtu;
	bool promisc_mode;
	bool all_mcast_mode;
	u32 pause_time;
};

/*  GSWIP-O Top Register write */
static inline void sw_write(struct gswip_mac *priv, u32 reg, u32 val)
{
	writel(val, priv->sw + reg);
}

/* GSWIP-O Top Register read */
static inline int sw_read(struct gswip_mac *priv, u32 reg)
{
	return readl(priv->sw + reg);
}

/* Legacy MAC Register read */
static inline void lmac_write(struct gswip_mac *priv, u32 reg, u32 val)
{
	writel(val, priv->lmac + reg);
}

/* Legacy MAC Register write */
static inline int lmac_read(struct gswip_mac *priv, u32 reg)
{
	return readl(priv->lmac + reg);
}

/* prototype */
void mac_init_ops(struct device *dev);
void xgmac_init_priv(struct gswip_mac *priv);
void xgmac_get_hw_features(struct gswip_mac *priv);
void xgmac_set_mac_address(struct gswip_mac *priv, u8 *mac_addr);
void xgmac_config_pkt(struct gswip_mac *priv, u32 mtu);
void xgmac_config_packet_filter(struct gswip_mac *priv, u32 mode);
void xgmac_tx_flow_ctl(struct gswip_mac *priv, u32 pause_time, u32 mode);
void xgmac_rx_flow_ctl(struct gswip_mac *priv, u32 mode);
int xgmac_set_mac_lpitx(struct gswip_mac *priv, u32 val);
int xgmac_enable(struct gswip_mac *priv);
int xgmac_disable(struct gswip_mac *priv);
int xgmac_pause_frame_filter(struct gswip_mac *priv, u32 val);
int xgmac_set_extcfg(struct gswip_mac *priv, u32 val);
int xgmac_set_xgmii_speed(struct gswip_mac *priv);
int xgmac_set_gmii_2500_speed(struct gswip_mac *priv);
int xgmac_set_xgmii_2500_speed(struct gswip_mac *priv);
int xgmac_set_gmii_speed(struct gswip_mac *priv);
int xgmac_mdio_set_clause(struct gswip_mac *priv, u32 clause, u32 phy_id);
int xgmac_mdio_register(struct gswip_mac *priv);

int sw_mac_set_mtu(struct gswip_mac *priv, u32 mtu);
u32 sw_mac_get_mtu(struct gswip_mac *priv);
u32 sw_get_speed(struct gswip_mac *priv);
int sw_set_flowctrl(struct gswip_mac *priv, u8 val, u32 mode);
int sw_get_linkstatus(struct gswip_mac *priv);
int sw_set_linkstatus(struct gswip_mac *priv, u8 linkst);
int sw_get_duplex_mode(struct gswip_mac *priv);
int sw_set_duplex_mode(struct gswip_mac *priv, u32 val);
int sw_set_speed(struct gswip_mac *priv, u8 speed);
int sw_set_2G5_intf(struct gswip_mac *priv, u32 macif);
int sw_set_1g_intf(struct gswip_mac *priv, u32 macif);
int sw_set_fe_intf(struct gswip_mac *priv, u32 macif);
int sw_set_eee_cap(struct gswip_mac *priv, u32 val);
int sw_set_mac_rxfcs_op(struct gswip_mac *priv, u32 val);

int lmac_set_flowcon_mode(struct gswip_mac *priv, u32 val);
int lmac_set_duplex_mode(struct gswip_mac *priv, u32 val);
int lmac_set_intf_mode(struct gswip_mac *priv, u32 val);

int sw_core_enable(struct device *dev, u32 val);
#endif
