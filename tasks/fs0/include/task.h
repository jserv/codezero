/*
 * Copyright (C) 2008 Bahadir Balban
 */
#ifndef __FS0_TASK_H__
#define __FS0_TASK_H__

#include <lib/idpool.h>
#include <l4/lib/list.h>
#include <l4/api/kip.h>

#define __TASKNAME__			__VFSNAME__

#define TASK_FILES_MAX			32

/* Thread control block, fs0 portion */
struct tcb {
	l4id_t tid;
	unsigned long utcb_address;
	int utcb_mapped;		/* Set if we mapped their utcb */
	struct list_head list;
	int fd[TASK_FILES_MAX];
	struct id_pool *fdpool;
	struct vnode *curdir;
	struct vnode *rootdir;
};

static inline int task_is_utcb_mapped(struct tcb *t)
{
	return t->utcb_mapped;
}

struct tcb *find_task(int tid);
int init_task_data(void);

#endif /* __FS0_TASK_H__ */
