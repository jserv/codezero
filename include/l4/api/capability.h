/*
 * Syscall API for capability manipulation
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __API_CAPABILITY_H__
#define __API_CAPABILITY_H__

/* Capability syscall request types */
#define CAP_CONTROL_NCAPS		0x00000000
#define CAP_CONTROL_READ		0x00000001
#define CAP_CONTROL_SHARE		0x00000002
#define CAP_CONTROL_GRANT		0x00000003
#define CAP_CONTROL_REPLICATE		0x00000005
#define CAP_CONTROL_SPLIT		0x00000006
#define CAP_CONTROL_DEDUCE		0x00000007

#define CAP_SHARE_MASK			0x00000003
#define CAP_SHARE_SINGLE		0x00000001
#define CAP_SHARE_ALL			0x00000002

#define CAP_GRANT_MASK			0x0000000F
#define CAP_GRANT_SINGLE		0x00000001
#define CAP_GRANT_ALL			0x00000002
#define CAP_GRANT_IMMUTABLE		0x00000004

/* Task's primary capability list */
#define TASK_CAP_LIST(task)	\
	(&((task)->space->cap_list))

#endif /* __API_CAPABILITY_H__ */
