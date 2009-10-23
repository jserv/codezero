/*
 * Syscall API for capability manipulation
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __API_CAPABILITY_H__
#define __API_CAPABILITY_H__

/* Capability syscall request types */
#define CAP_CONTROL_NCAPS		0x00
#define CAP_CONTROL_READ		0x01
#define CAP_CONTROL_SHARE		0x02

#define CAP_SHARE_MASK			0x1F
#define CAP_SHARE_SPACE			0x01
#define CAP_SHARE_CONTAINER		0x02
#define CAP_SHARE_GROUP			0x04
#define CAP_SHARE_CHILD			0x08	/* All that we are pager of */
#define CAP_SHARE_SIBLING		0x10	/* All that have a common pager */

#endif /* __API_CAPABILITY_H__ */
