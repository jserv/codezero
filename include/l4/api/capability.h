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

#define CAP_SHARE_WITH_SPACE		1
#define CAP_SHARE_WITH_CONTAINER	2
#define CAP_SHARE_WITH_TGROUP		4

#endif /* __API_CAPABILITY_H__ */
