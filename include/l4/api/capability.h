/*
 * Syscall API for capability manipulation
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __API_CAPABILITY_H__
#define __API_CAPABILITY_H__

/* Capability syscall request types */
#define CAP_CONTROL_NCAPS		0
#define CAP_CONTROL_READ		1
#define CAP_CONTROL_SHARE		2

#define CAP_SHARE_SPACE		1
#define CAP_SHARE_CONTAINER	2
#define CAP_SHARE_GROUP		4
#define CAP_SHARE_PAGED		8	/* All that we are pager of */

#endif /* __API_CAPABILITY_H__ */
