// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Intel Corporation */

#include "idpf_dev.h"
#include <linux/net/intel/iecm_lan_pf_regs.h>

/**
 * idpf_ctlq_reg_init - initialize default mailbox registers
 * @cq: pointer to the array of create control queues
 */
void idpf_ctlq_reg_init(struct iecm_ctlq_create_info *cq)
{
	int i;

#define NUM_Q 2
	for (i = 0; i < NUM_Q; i++) {
		struct iecm_ctlq_create_info *ccq = cq + i;

		switch (ccq->type) {
		case IECM_CTLQ_TYPE_MAILBOX_TX:
			/* set head and tail registers in our local struct */
			ccq->reg.head = PF_FW_ATQH;
			ccq->reg.tail = PF_FW_ATQT;
			ccq->reg.len = PF_FW_ATQLEN;
			ccq->reg.bah = PF_FW_ATQBAH;
			ccq->reg.bal = PF_FW_ATQBAL;
			ccq->reg.len_mask = PF_FW_ATQLEN_ATQLEN_M;
			ccq->reg.len_ena_mask = PF_FW_ATQLEN_ATQENABLE_M;
			ccq->reg.head_mask = PF_FW_ATQH_ATQH_M;
			break;
		case IECM_CTLQ_TYPE_MAILBOX_RX:
			/* set head and tail registers in our local struct */
			ccq->reg.head = PF_FW_ARQH;
			ccq->reg.tail = PF_FW_ARQT;
			ccq->reg.len = PF_FW_ARQLEN;
			ccq->reg.bah = PF_FW_ARQBAH;
			ccq->reg.bal = PF_FW_ARQBAL;
			ccq->reg.len_mask = PF_FW_ARQLEN_ARQLEN_M;
			ccq->reg.len_ena_mask = PF_FW_ARQLEN_ARQENABLE_M;
			ccq->reg.head_mask = PF_FW_ARQH_ARQH_M;
			break;
		default:
			break;
		}
	}
}

/**
 * idpf_vportq_reg_init - Initialize tail registers
 * @vport: virtual port structure
 */
void idpf_vportq_reg_init(struct iecm_vport *vport)
{
	struct iecm_hw *hw = &vport->adapter->hw;
	struct iecm_queue *q;
	int i, j;

	for (i = 0; i < vport->num_txq_grp; i++) {
		int num_txq = vport->txq_grps[i].num_txq;

		for (j = 0; j < num_txq; j++) {
			q = &vport->txq_grps[i].txqs[j];
			q->tail = hw->hw_addr + PF_QTX_COMM_DBELL(q->q_id);
		}
	}

	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct iecm_rxq_group *rxq_grp = &vport->rxq_grps[i];
		int num_rxq;

		if (iecm_is_queue_model_split(vport->rxq_model)) {
			for (j = 0; j < IECM_BUFQS_PER_RXQ_SET; j++) {
				q = &rxq_grp->splitq.bufq_sets[j].bufq;
				q->tail = hw->hw_addr +
					  PF_QRX_BUFFQ_TAIL(q->q_id);
			}

			num_rxq = rxq_grp->splitq.num_rxq_sets;
		} else {
			num_rxq = rxq_grp->singleq.num_rxq;
		}

		for (j = 0; j < num_rxq; j++) {
			if (iecm_is_queue_model_split(vport->rxq_model))
				q = &rxq_grp->splitq.rxq_sets[j].rxq;
			else
				q = &rxq_grp->singleq.rxqs[j];
			q->tail = hw->hw_addr + PF_QRX_TAIL(q->q_id);
		}
	}
}

/**
 * idpf_mb_intr_reg_init - Initialize mailbox interrupt register
 * @adapter: adapter structure
 */
void idpf_mb_intr_reg_init(struct iecm_adapter *adapter)
{
	struct iecm_intr_reg *intr = &adapter->mb_vector.intr_reg;
	int vidx;

	vidx = adapter->mb_vector.v_idx;
	intr->dyn_ctl = PF_GLINT_DYN_CTL(vidx);
	intr->dyn_ctl_intena_m = PF_GLINT_DYN_CTL_INTENA_M;
	intr->dyn_ctl_itridx_m = 0x3 << PF_GLINT_DYN_CTL_ITR_INDX_S;
}

/**
 * idpf_intr_reg_init - Initialize interrupt registers
 * @vport: virtual port structure
 */
void idpf_intr_reg_init(struct iecm_vport *vport)
{
	int q_idx;

	for (q_idx = 0; q_idx < vport->num_q_vectors; q_idx++) {
		struct iecm_q_vector *q_vector = &vport->q_vectors[q_idx];
		struct iecm_intr_reg *intr = &q_vector->intr_reg;
		u32 vidx = q_vector->v_idx;

		intr->dyn_ctl = PF_GLINT_DYN_CTL(vidx);
		intr->dyn_ctl_clrpba_m = PF_GLINT_DYN_CTL_CLEARPBA_M;
		intr->dyn_ctl_intena_m = PF_GLINT_DYN_CTL_INTENA_M;
		intr->dyn_ctl_itridx_s = PF_GLINT_DYN_CTL_ITR_INDX_S;
		intr->dyn_ctl_intrvl_s = PF_GLINT_DYN_CTL_INTERVAL_S;
		intr->itr = PF_GLINT_ITR(VIRTCHNL_ITR_IDX_0, vidx);
	}
}

/**
 * idpf_reset_reg_init - Initialize reset registers
 * @reset_reg: struct to be filled in with reset registers
 */
void idpf_reset_reg_init(struct iecm_reset_reg *reset_reg)
{
	reset_reg->rstat = PFGEN_RSTAT;
	reset_reg->rstat_m = PFGEN_RSTAT_PFR_STATE_M;
}

/**
 * idpf_trigger_reset - trigger reset
 * @adapter: Driver specific private structure
 * @trig_cause: Reason to trigger a reset
 */
void idpf_trigger_reset(struct iecm_adapter *adapter,
			enum iecm_flags __always_unused trig_cause)
{
	u32 reset_reg;

	reset_reg = rd32(&adapter->hw, PFGEN_CTRL);
	wr32(&adapter->hw, PFGEN_CTRL, (reset_reg | PFGEN_CTRL_PFSWR));
}
