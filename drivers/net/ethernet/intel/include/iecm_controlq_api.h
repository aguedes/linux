/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020, Intel Corporation. */

#ifndef _IECM_CONTROLQ_API_H_
#define _IECM_CONTROLQ_API_H_

/* Error Codes */
enum iecm_status {
	IECM_SUCCESS				= 0,

	/* Reserve range -1..-49 for generic codes */
	IECM_ERR_PARAM				= -1,
	IECM_ERR_NOT_IMPL			= -2,
	IECM_ERR_NOT_READY			= -3,
	IECM_ERR_BAD_PTR			= -5,
	IECM_ERR_INVAL_SIZE			= -6,
	IECM_ERR_DEVICE_NOT_SUPPORTED		= -8,
	IECM_ERR_FW_API_VER			= -9,
	IECM_ERR_NO_MEMORY			= -10,
	IECM_ERR_CFG				= -11,
	IECM_ERR_OUT_OF_RANGE			= -12,
	IECM_ERR_ALREADY_EXISTS			= -13,
	IECM_ERR_DOES_NOT_EXIST			= -14,
	IECM_ERR_IN_USE				= -15,
	IECM_ERR_MAX_LIMIT			= -16,
	IECM_ERR_RESET_ONGOING			= -17,

	/* Reserve range -100..-109 for CRQ/CSQ specific error codes */
	IECM_ERR_CTLQ_ERROR			= -100,
	IECM_ERR_CTLQ_TIMEOUT			= -101,
	IECM_ERR_CTLQ_FULL			= -102,
	IECM_ERR_CTLQ_NO_WORK			= -103,
	IECM_ERR_CTLQ_EMPTY			= -104,
};

enum iecm_ctlq_err {
	IECM_CTLQ_RC_OK		= 0,  /* Success */
	IECM_CTLQ_RC_EPERM	= 1,  /* Operation not permitted */
	IECM_CTLQ_RC_ENOENT	= 2,  /* No such element */
	IECM_CTLQ_RC_ESRCH	= 3,  /* Bad opcode */
	IECM_CTLQ_RC_EINTR	= 4,  /* Operation interrupted */
	IECM_CTLQ_RC_EIO	= 5,  /* I/O error */
	IECM_CTLQ_RC_ENXIO	= 6,  /* No such resource */
	IECM_CTLQ_RC_E2BIG	= 7,  /* Arg too long */
	IECM_CTLQ_RC_EAGAIN	= 8,  /* Try again */
	IECM_CTLQ_RC_ENOMEM	= 9,  /* Out of memory */
	IECM_CTLQ_RC_EACCES	= 10, /* Permission denied */
	IECM_CTLQ_RC_EFAULT	= 11, /* Bad address */
	IECM_CTLQ_RC_EBUSY	= 12, /* Device or resource busy */
	IECM_CTLQ_RC_EEXIST	= 13, /* object already exists */
	IECM_CTLQ_RC_EINVAL	= 14, /* Invalid argument */
	IECM_CTLQ_RC_ENOTTY	= 15, /* Not a typewriter */
	IECM_CTLQ_RC_ENOSPC	= 16, /* No space left or allocation failure */
	IECM_CTLQ_RC_ENOSYS	= 17, /* Function not implemented */
	IECM_CTLQ_RC_ERANGE	= 18, /* Parameter out of range */
	IECM_CTLQ_RC_EFLUSHED	= 19, /* Cmd flushed due to prev cmd error */
	IECM_CTLQ_RC_BAD_ADDR	= 20, /* Descriptor contains a bad pointer */
	IECM_CTLQ_RC_EMODE	= 21, /* Op not allowed in current dev mode */
	IECM_CTLQ_RC_EFBIG	= 22, /* File too big */
	IECM_CTLQ_RC_ENOSEC	= 24, /* Missing security manifest */
	IECM_CTLQ_RC_EBADSIG	= 25, /* Bad RSA signature */
	IECM_CTLQ_RC_ESVN	= 26, /* SVN number prohibits this package */
	IECM_CTLQ_RC_EBADMAN	= 27, /* Manifest hash mismatch */
	IECM_CTLQ_RC_EBADBUF	= 28, /* Buffer hash mismatches manifest */
};

/* Used for queue init, response and events */
enum iecm_ctlq_type {
	IECM_CTLQ_TYPE_MAILBOX_TX	= 0,
	IECM_CTLQ_TYPE_MAILBOX_RX	= 1,
	IECM_CTLQ_TYPE_CONFIG_TX	= 2,
	IECM_CTLQ_TYPE_CONFIG_RX	= 3,
	IECM_CTLQ_TYPE_EVENT_RX		= 4,
	IECM_CTLQ_TYPE_RDMA_TX		= 5,
	IECM_CTLQ_TYPE_RDMA_RX		= 6,
	IECM_CTLQ_TYPE_RDMA_COMPL	= 7
};

/* Generic Control Queue Structures */

struct iecm_ctlq_reg {
	/* used for queue tracking */
	u32 head;
	u32 tail;
	/* Below applies only to default mb (if present) */
	u32 len;
	u32 bah;
	u32 bal;
	u32 len_mask;
	u32 len_ena_mask;
	u32 head_mask;
};

/* Generic queue msg structure */
struct iecm_ctlq_msg {
	u16 vmvf_type; /* represents the source of the message on recv */
#define IECM_VMVF_TYPE_VF 0
#define IECM_VMVF_TYPE_VM 1
#define IECM_VMVF_TYPE_PF 2
	u16 opcode;
	u16 data_len;	/* data_len = 0 when no payload is attached */
	union {
		u16 func_id;	/* when sending a message */
		u16 status;	/* when receiving a message */
	};
	union {
		struct {
			u32 chnl_retval;
			u32 chnl_opcode;
		} mbx;
	} cookie;
	union {
#define IECM_DIRECT_CTX_SIZE	16
#define IECM_INDIRECT_CTX_SIZE	8
		/* 16 bytes of context can be provided or 8 bytes of context
		 * plus the address of a DMA buffer
		 */
		u8 direct[IECM_DIRECT_CTX_SIZE];
		struct {
			u8 context[IECM_INDIRECT_CTX_SIZE];
			struct iecm_dma_mem *payload;
		} indirect;
	} ctx;
};

/* Generic queue info structures */
/* MB, CONFIG and EVENT q do not have extended info */
struct iecm_ctlq_create_info {
	enum iecm_ctlq_type type;
	/* absolute queue offset passed as input
	 * -1 for default mailbox if present
	 */
	int id;
	u16 len; /* Queue length passed as input */
	u16 buf_size; /* buffer size passed as input */
	u64 base_address; /* output, HPA of the Queue start  */
	struct iecm_ctlq_reg reg; /* registers accessed by ctlqs */

	int ext_info_size;
	void *ext_info; /* Specific to q type */
};

/* Control Queue information */
struct iecm_ctlq_info {
	struct list_head cq_list;

	enum iecm_ctlq_type cq_type;
	int q_id;
	struct mutex cq_lock;		/* queue lock */

	/* used for interrupt processing */
	u16 next_to_use;
	u16 next_to_clean;

	/* starting descriptor to post buffers to after recev */
	u16 next_to_post;
	struct iecm_dma_mem desc_ring;	/* descriptor ring memory */

	union {
		struct iecm_dma_mem **rx_buff;
		struct iecm_ctlq_msg **tx_msg;
	} bi;

	u16 buf_size;			/* queue buffer size */
	u16 ring_size;			/* Number of descriptors */
	struct iecm_ctlq_reg reg;	/* registers accessed by ctlqs */
};

/* PF/VF mailbox commands */
enum iecm_mbx_opc {
	iecm_mbq_opc_send_msg_to_cp		= 0x0801,
	iecm_mbq_opc_send_msg_to_vf		= 0x0802,
	iecm_mbq_opc_send_msg_to_pf		= 0x0803,
};

/* API supported for control queue management */

/* Will init all required q including default mb.  "q_info" is an array of
 * create_info structs equal to the number of control queues to be created.
 */
enum iecm_status iecm_ctlq_init(struct iecm_hw *hw, u8 num_q,
				struct iecm_ctlq_create_info *q_info);

/* Allocate and initialize a single control queue, which will be added to the
 * control queue list; returns a handle to the created control queue
 */
enum iecm_status iecm_ctlq_add(struct iecm_hw *hw,
			       struct iecm_ctlq_create_info *qinfo,
			       struct iecm_ctlq_info **cq);
/* Deinitialize and deallocate a single control queue */
void iecm_ctlq_remove(struct iecm_hw *hw,
		      struct iecm_ctlq_info *cq);

/* Sends messages to HW and will also free the buffer*/
enum iecm_status iecm_ctlq_send(struct iecm_hw *hw,
				struct iecm_ctlq_info *cq,
				u16 num_q_msg,
				struct iecm_ctlq_msg q_msg[]);

/* Receives messages and called by interrupt handler/polling
 * initiated by app/process. Also caller is supposed to free the buffers
 */
enum iecm_status iecm_ctlq_recv(struct iecm_hw *hw,
				struct iecm_ctlq_info *cq,
				u16 *num_q_msg,	struct iecm_ctlq_msg *q_msg);
/* Reclaims send descriptors on HW write back */
enum iecm_status iecm_ctlq_clean_sq(struct iecm_hw *hw,
				    struct iecm_ctlq_info *cq,
				    u16 *clean_count,
				    struct iecm_ctlq_msg *msg_status[]);

/* Indicate RX buffers are done being processed */
enum iecm_status iecm_ctlq_post_rx_buffs(struct iecm_hw *hw,
					 struct iecm_ctlq_info *cq,
					 u16 *buff_count,
					 struct iecm_dma_mem **buffs);

/* Will destroy all q including the default mb */
enum iecm_status iecm_ctlq_deinit(struct iecm_hw *hw);

#endif /* _IECM_CONTROLQ_API_H_ */
