// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020, Intel Corporation. */

#include "ice_acl.h"

/**
 * ice_aq_alloc_acl_tbl - allocate ACL table
 * @hw: pointer to the HW struct
 * @tbl: pointer to ice_acl_alloc_tbl struct
 * @cd: pointer to command details structure or NULL
 *
 * Allocate ACL table (indirect 0x0C10)
 */
enum ice_status
ice_aq_alloc_acl_tbl(struct ice_hw *hw, struct ice_acl_alloc_tbl *tbl,
		     struct ice_sq_cd *cd)
{
	struct ice_aqc_acl_alloc_table *cmd;
	struct ice_aq_desc desc;

	if (!tbl->act_pairs_per_entry)
		return ICE_ERR_PARAM;

	if (tbl->act_pairs_per_entry > ICE_AQC_MAX_ACTION_MEMORIES)
		return ICE_ERR_MAX_LIMIT;

	/* If this is concurrent table, then buffer shall be valid and
	 * contain DependentAllocIDs, 'num_dependent_alloc_ids' should be valid
	 * and within limit
	 */
	if (tbl->concurr) {
		if (!tbl->num_dependent_alloc_ids)
			return ICE_ERR_PARAM;
		if (tbl->num_dependent_alloc_ids >
		    ICE_AQC_MAX_CONCURRENT_ACL_TBL)
			return ICE_ERR_INVAL_SIZE;
	}

	ice_fill_dflt_direct_cmd_desc(&desc, ice_aqc_opc_alloc_acl_tbl);
	desc.flags |= cpu_to_le16(ICE_AQ_FLAG_RD);

	cmd = &desc.params.alloc_table;
	cmd->table_width = cpu_to_le16(tbl->width * BITS_PER_BYTE);
	cmd->table_depth = cpu_to_le16(tbl->depth);
	cmd->act_pairs_per_entry = tbl->act_pairs_per_entry;
	if (tbl->concurr)
		cmd->table_type = tbl->num_dependent_alloc_ids;

	return ice_aq_send_cmd(hw, &desc, &tbl->buf, sizeof(tbl->buf), cd);
}

/**
 * ice_aq_dealloc_acl_tbl - deallocate ACL table
 * @hw: pointer to the HW struct
 * @alloc_id: allocation ID of the table being released
 * @buf: address of indirect data buffer
 * @cd: pointer to command details structure or NULL
 *
 * Deallocate ACL table (indirect 0x0C11)
 *
 * NOTE: This command has no buffer format for command itself but response
 * format is 'struct ice_aqc_acl_generic', pass ptr to that struct
 * as 'buf' and its size as 'buf_size'
 */
enum ice_status
ice_aq_dealloc_acl_tbl(struct ice_hw *hw, u16 alloc_id,
		       struct ice_aqc_acl_generic *buf, struct ice_sq_cd *cd)
{
	struct ice_aqc_acl_tbl_actpair *cmd;
	struct ice_aq_desc desc;

	ice_fill_dflt_direct_cmd_desc(&desc, ice_aqc_opc_dealloc_acl_tbl);
	cmd = &desc.params.tbl_actpair;
	cmd->alloc_id = cpu_to_le16(alloc_id);

	return ice_aq_send_cmd(hw, &desc, buf, sizeof(*buf), cd);
}

static enum ice_status
ice_aq_acl_entry(struct ice_hw *hw, u16 opcode, u8 tcam_idx, u16 entry_idx,
		 struct ice_aqc_acl_data *buf, struct ice_sq_cd *cd)
{
	struct ice_aqc_acl_entry *cmd;
	struct ice_aq_desc desc;

	ice_fill_dflt_direct_cmd_desc(&desc, opcode);

	if (opcode == ice_aqc_opc_program_acl_entry)
		desc.flags |= cpu_to_le16(ICE_AQ_FLAG_RD);

	cmd = &desc.params.program_query_entry;
	cmd->tcam_index = tcam_idx;
	cmd->entry_index = cpu_to_le16(entry_idx);

	return ice_aq_send_cmd(hw, &desc, buf, sizeof(*buf), cd);
}

/**
 * ice_aq_program_acl_entry - program ACL entry
 * @hw: pointer to the HW struct
 * @tcam_idx: Updated TCAM block index
 * @entry_idx: updated entry index
 * @buf: address of indirect data buffer
 * @cd: pointer to command details structure or NULL
 *
 * Program ACL entry (direct 0x0C20)
 */
enum ice_status
ice_aq_program_acl_entry(struct ice_hw *hw, u8 tcam_idx, u16 entry_idx,
			 struct ice_aqc_acl_data *buf, struct ice_sq_cd *cd)
{
	return ice_aq_acl_entry(hw, ice_aqc_opc_program_acl_entry, tcam_idx,
				entry_idx, buf, cd);
}

/* Helper function to program/query ACL action pair */
static enum ice_status
ice_aq_actpair_p_q(struct ice_hw *hw, u16 opcode, u8 act_mem_idx,
		   u16 act_entry_idx, struct ice_aqc_actpair *buf,
		   struct ice_sq_cd *cd)
{
	struct ice_aqc_acl_actpair *cmd;
	struct ice_aq_desc desc;

	ice_fill_dflt_direct_cmd_desc(&desc, opcode);

	if (opcode == ice_aqc_opc_program_acl_actpair)
		desc.flags |= cpu_to_le16(ICE_AQ_FLAG_RD);

	cmd = &desc.params.program_query_actpair;
	cmd->act_mem_index = act_mem_idx;
	cmd->act_entry_index = cpu_to_le16(act_entry_idx);

	return ice_aq_send_cmd(hw, &desc, buf, sizeof(*buf), cd);
}

/**
 * ice_aq_program_actpair - program ACL actionpair
 * @hw: pointer to the HW struct
 * @act_mem_idx: action memory index to program/update/query
 * @act_entry_idx: the entry index in action memory to be programmed/updated
 * @buf: address of indirect data buffer
 * @cd: pointer to command details structure or NULL
 *
 * Program action entries (indirect 0x0C1C)
 */
enum ice_status
ice_aq_program_actpair(struct ice_hw *hw, u8 act_mem_idx, u16 act_entry_idx,
		       struct ice_aqc_actpair *buf, struct ice_sq_cd *cd)
{
	return ice_aq_actpair_p_q(hw, ice_aqc_opc_program_acl_actpair,
				  act_mem_idx, act_entry_idx, buf, cd);
}
