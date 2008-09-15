/*
 * Userspace thread control block
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __GLUE_ARM_MESSAGE_H__
#define __GLUE_ARM_MESSAGE_H__

#define MR_TOTAL		6
#define MR_TAG			0	/* Contains the purpose of message */
#define MR_SENDER		1	/* For anythread receivers to discover sender */
#define MR_RETURN		0	/* Contains the posix return value. */

/* These define the mr start - end range that isn't used by userspace syslib */
#define MR_UNUSED_START		2	/* The first mr that's not used by syslib.h */
#define MR_UNUSED_TOTAL		(MR_TOTAL - MR_UNUSED_START)
#define MR_USABLE_TOTAL		MR_UNUSED_TOTAL

/* These are defined so that we don't hard-code register names */
#define MR0_REGISTER		r3
#define MR_RETURN_REGISTER	r3

#endif /* __GLUE_ARM_MESSAGE_H__ */
