/*
 * Userspace thread control block
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __GLUE_ARM_UTCB_H__
#define __GLUE_ARM_UTCB_H__

#define MR_TOTAL		6

/* Compact utcb for now! 8-) */
struct utcb {
	u32 mr[MR_TOTAL];
	u32 global_id;		/* Thread id */
	u32 usr_handle;		/* Use as TLS */
};

#endif /* __GLUE_ARM_UTCB_H__ */
