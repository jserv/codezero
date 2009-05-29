#ifndef __MUTEX_CONTROL_H__
#define __MUTEX_CONTROL_H__

/* Request ids for mutex_control syscall */

#if defined (__KERNEL__)
#define MUTEX_CONTROL_LOCK		L4_MUTEX_LOCK
#define MUTEX_CONTROL_UNLOCK		L4_MUTEX_UNLOCK
#endif

#define L4_MUTEX_LOCK		0
#define L4_MUTEX_UNLOCK		1

#endif /* __MUTEX_CONTROL_H__*/
