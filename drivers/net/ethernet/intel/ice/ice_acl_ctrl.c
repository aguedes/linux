// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020, Intel Corporation. */

#include "ice_acl.h"

/* Determine the TCAM index of entry 'e' within the ACL table */
#define ICE_ACL_TBL_TCAM_IDX(e) ((e) / ICE_AQC_ACL_TCAM_DEPTH)

/**
 * ice_acl_init_tbl
 * @hw: pointer to the hardware structure
 *
 * Initialize the ACL table by invalidating TCAM entries and action pairs.
 */
static enum ice_status ice_acl_init_tbl(struct ice_hw *hw)
{
	struct ice_aqc_actpair act_buf;
	struct ice_aqc_acl_data buf;
	enum ice_status status = 0;
	struct ice_acl_tbl *tbl;
	u8 tcam_idx, i;
	u16 idx;

	tbl = hw->acl_tbl;
	if (!tbl)
		return ICE_ERR_CFG;

	memset(&buf, 0, sizeof(buf));
	memset(&act_buf, 0, sizeof(act_buf));

	tcam_idx = tbl->first_tcam;
	idx = tbl->first_entry;
	while (tcam_idx < tbl->last_tcam ||
	       (tcam_idx == tbl->last_tcam && idx <= tbl->last_entry)) {
		/* Use the same value for entry_key and entry_key_inv since
		 * we are initializing the fields to 0
		 */
		status = ice_aq_program_acl_entry(hw, tcam_idx, idx, &buf,
						  NULL);
		if (status)
			return status;

		if (++idx > tbl->last_entry) {
			tcam_idx++;
			idx = tbl->first_entry;
		}
	}

	for (i = 0; i < ICE_AQC_MAX_ACTION_MEMORIES; i++) {
		u16 act_entry_idx, start, end;

		if (tbl->act_mems[i].act_mem == ICE_ACL_ACT_PAIR_MEM_INVAL)
			continue;

		start = tbl->first_entry;
		end = tbl->last_entry;

		for (act_entry_idx = start; act_entry_idx <= end;
		     act_entry_idx++) {
			/* Invalidate all allocated action pairs */
			status = ice_aq_program_actpair(hw, i, act_entry_idx,
							&act_buf, NULL);
			if (status)
				return status;
		}
	}

	return status;
}

/**
 * ice_acl_assign_act_mems_to_tcam
 * @tbl: pointer to ACL table structure
 * @cur_tcam: Index of current TCAM. Value = 0 to (ICE_AQC_ACL_SLICES - 1)
 * @cur_mem_idx: Index of current action memory bank. Value = 0 to
 *		 (ICE_AQC_MAX_ACTION_MEMORIES - 1)
 * @num_mem: Number of action memory banks for this TCAM
 *
 * Assign "num_mem" valid action memory banks from "curr_mem_idx" to
 * "curr_tcam" TCAM.
 */
static void
ice_acl_assign_act_mems_to_tcam(struct ice_acl_tbl *tbl, u8 cur_tcam,
				u8 *cur_mem_idx, u8 num_mem)
{
	u8 mem_cnt;

	for (mem_cnt = 0;
	     *cur_mem_idx < ICE_AQC_MAX_ACTION_MEMORIES && mem_cnt < num_mem;
	     (*cur_mem_idx)++) {
		struct ice_acl_act_mem *p_mem = &tbl->act_mems[*cur_mem_idx];

		if (p_mem->act_mem == ICE_ACL_ACT_PAIR_MEM_INVAL)
			continue;

		p_mem->member_of_tcam = cur_tcam;

		mem_cnt++;
	}
}

/**
 * ice_acl_divide_act_mems_to_tcams
 * @tbl: pointer to ACL table structure
 *
 * Figure out how to divide given action memory banks to given TCAMs. This
 * division is for SW book keeping. In the time when scenario is created,
 * an action memory bank can be used for different TCAM.
 *
 * For example, given that we have 2x2 ACL table with each table entry has
 * 2 action memory pairs. As the result, we will have 4 TCAMs (T1,T2,T3,T4)
 * and 4 action memory banks (A1,A2,A3,A4)
 *	[T1 - T2] { A1 - A2 }
 *	[T3 - T4] { A3 - A4 }
 * In the time when we need to create a scenario, for example, 2x1 scenario,
 * we will use [T3,T4] in a cascaded layout. As it is a requirement that all
 * action memory banks in a cascaded TCAM's row will need to associate with
 * the last TCAM. Thus, we will associate action memory banks [A3] and [A4]
 * for TCAM [T4].
 * For SW book-keeping purpose, we will keep theoretical maps between TCAM
 * [Tn] to action memory bank [An].
 */
static void ice_acl_divide_act_mems_to_tcams(struct ice_acl_tbl *tbl)
{
	u16 num_cscd, stack_level, stack_idx, min_act_mem;
	u8 tcam_idx = tbl->first_tcam;
	u16 max_idx_to_get_extra;
	u8 mem_idx = 0;

	/* Determine number of stacked TCAMs */
	stack_level = DIV_ROUND_UP(tbl->info.depth, ICE_AQC_ACL_TCAM_DEPTH);

	/* Determine number of cascaded TCAMs */
	num_cscd = DIV_ROUND_UP(tbl->info.width, ICE_AQC_ACL_KEY_WIDTH_BYTES);

	/* In a line of cascaded TCAM, given the number of action memory
	 * banks per ACL table entry, we want to fairly divide these action
	 * memory banks between these TCAMs.
	 *
	 * For example, there are 3 TCAMs (TCAM 3,4,5) in a line of
	 * cascaded TCAM, and there are 7 act_mems for each ACL table entry.
	 * The result is:
	 *	[TCAM_3 will have 3 act_mems]
	 *	[TCAM_4 will have 2 act_mems]
	 *	[TCAM_5 will have 2 act_mems]
	 */
	min_act_mem = tbl->info.entry_act_pairs / num_cscd;
	max_idx_to_get_extra = tbl->info.entry_act_pairs % num_cscd;

	for (stack_idx = 0; stack_idx < stack_level; stack_idx++) {
		u16 i;

		for (i = 0; i < num_cscd; i++) {
			u8 total_act_mem = min_act_mem;

			if (i < max_idx_to_get_extra)
				total_act_mem++;

			ice_acl_assign_act_mems_to_tcam(tbl, tcam_idx,
							&mem_idx,
							total_act_mem);

			tcam_idx++;
		}
	}
}

/**
 * ice_acl_create_tbl
 * @hw: pointer to the HW struct
 * @params: parameters for the table to be created
 *
 * Create a LEM table for ACL usage. We are currently starting with some fixed
 * values for the size of the table, but this will need to grow as more flow
 * entries are added by the user level.
 */
enum ice_status
ice_acl_create_tbl(struct ice_hw *hw, struct ice_acl_tbl_params *params)
{
	u16 width, depth, first_e, last_e, i;
	struct ice_aqc_acl_generic *resp_buf;
	struct ice_acl_alloc_tbl tbl_alloc;
	struct ice_acl_tbl *tbl;
	enum ice_status status;

	if (hw->acl_tbl)
		return ICE_ERR_ALREADY_EXISTS;

	if (!params)
		return ICE_ERR_PARAM;

	/* round up the width to the next TCAM width boundary. */
	width = roundup(params->width, (u16)ICE_AQC_ACL_KEY_WIDTH_BYTES);
	/* depth should be provided in chunk (64 entry) increments */
	depth = ALIGN(params->depth, ICE_ACL_ENTRY_ALLOC_UNIT);

	if (params->entry_act_pairs < width / ICE_AQC_ACL_KEY_WIDTH_BYTES) {
		params->entry_act_pairs = width / ICE_AQC_ACL_KEY_WIDTH_BYTES;

		if (params->entry_act_pairs > ICE_AQC_TBL_MAX_ACTION_PAIRS)
			params->entry_act_pairs = ICE_AQC_TBL_MAX_ACTION_PAIRS;
	}

	/* Validate that width*depth will not exceed the TCAM limit */
	if ((DIV_ROUND_UP(depth, ICE_AQC_ACL_TCAM_DEPTH) *
	     (width / ICE_AQC_ACL_KEY_WIDTH_BYTES)) > ICE_AQC_ACL_SLICES)
		return ICE_ERR_MAX_LIMIT;

	memset(&tbl_alloc, 0, sizeof(tbl_alloc));
	tbl_alloc.width = width;
	tbl_alloc.depth = depth;
	tbl_alloc.act_pairs_per_entry = params->entry_act_pairs;
	tbl_alloc.concurr = params->concurr;
	/* Set dependent_alloc_id only for concurrent table type */
	if (params->concurr) {
		tbl_alloc.num_dependent_alloc_ids =
			ICE_AQC_MAX_CONCURRENT_ACL_TBL;

		for (i = 0; i < ICE_AQC_MAX_CONCURRENT_ACL_TBL; i++)
			tbl_alloc.buf.data_buf.alloc_ids[i] =
				cpu_to_le16(params->dep_tbls[i]);
	}

	/* call the AQ command to create the ACL table with these values */
	status = ice_aq_alloc_acl_tbl(hw, &tbl_alloc, NULL);
	if (status) {
		if (le16_to_cpu(tbl_alloc.buf.resp_buf.alloc_id) <
		    ICE_AQC_ALLOC_ID_LESS_THAN_4K)
			ice_debug(hw, ICE_DBG_ACL, "Alloc ACL table failed. Unavailable resource.\n");
		else
			ice_debug(hw, ICE_DBG_ACL, "AQ allocation of ACL failed with error. status: %d\n",
				  status);
		return status;
	}

	tbl = devm_kzalloc(ice_hw_to_dev(hw), sizeof(*tbl), GFP_KERNEL);
	if (!tbl)
		return ICE_ERR_NO_MEMORY;

	resp_buf = &tbl_alloc.buf.resp_buf;

	/* Retrieve information of the allocated table */
	tbl->id = le16_to_cpu(resp_buf->alloc_id);
	tbl->first_tcam = resp_buf->ops.table.first_tcam;
	tbl->last_tcam = resp_buf->ops.table.last_tcam;
	tbl->first_entry = le16_to_cpu(resp_buf->first_entry);
	tbl->last_entry = le16_to_cpu(resp_buf->last_entry);

	tbl->info = *params;
	tbl->info.width = width;
	tbl->info.depth = depth;
	hw->acl_tbl = tbl;

	for (i = 0; i < ICE_AQC_MAX_ACTION_MEMORIES; i++)
		tbl->act_mems[i].act_mem = resp_buf->act_mem[i];

	/* Figure out which TCAMs that these newly allocated action memories
	 * belong to.
	 */
	ice_acl_divide_act_mems_to_tcams(tbl);

	/* Initialize the resources allocated by invalidating all TCAM entries
	 * and all the action pairs
	 */
	status = ice_acl_init_tbl(hw);
	if (status) {
		devm_kfree(ice_hw_to_dev(hw), tbl);
		hw->acl_tbl = NULL;
		ice_debug(hw, ICE_DBG_ACL, "Initialization of TCAM entries failed. status: %d\n",
			  status);
		return status;
	}

	first_e = (tbl->first_tcam * ICE_AQC_MAX_TCAM_ALLOC_UNITS) +
		(tbl->first_entry / ICE_ACL_ENTRY_ALLOC_UNIT);
	last_e = (tbl->last_tcam * ICE_AQC_MAX_TCAM_ALLOC_UNITS) +
		(tbl->last_entry / ICE_ACL_ENTRY_ALLOC_UNIT);

	/* Indicate available entries in the table */
	bitmap_set(tbl->avail, first_e, last_e - first_e + 1);

	INIT_LIST_HEAD(&tbl->scens);

	return 0;
}

/**
 * ice_acl_destroy_tbl - Destroy a previously created LEM table for ACL
 * @hw: pointer to the HW struct
 */
enum ice_status ice_acl_destroy_tbl(struct ice_hw *hw)
{
	struct ice_aqc_acl_generic resp_buf;
	enum ice_status status;

	if (!hw->acl_tbl)
		return ICE_ERR_DOES_NOT_EXIST;

	/* call the AQ command to destroy the ACL table */
	status = ice_aq_dealloc_acl_tbl(hw, hw->acl_tbl->id, &resp_buf, NULL);
	if (status) {
		ice_debug(hw, ICE_DBG_ACL, "AQ de-allocation of ACL failed. status: %d\n",
			  status);
		return status;
	}

	devm_kfree(ice_hw_to_dev(hw), hw->acl_tbl);
	hw->acl_tbl = NULL;

	return 0;
}
