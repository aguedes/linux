# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2020 Intel Corporation

#
# Makefile for the Intel(R) Ethernet Common Module
#

ccflags-y += -I$(srctree)/drivers/net/ethernet/intel/include

obj-$(CONFIG_IECM) += iecm.o

iecm-y := \
	iecm_lib.o \
	iecm_virtchnl.o \
	iecm_txrx.o \
	iecm_singleq_txrx.o \
	iecm_ethtool.o \
	iecm_controlq.o \
	iecm_osdep.o \
	iecm_controlq_setup.o \
	iecm_main.o
