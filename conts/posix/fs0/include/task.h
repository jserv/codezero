/*
 * Copyright (C) 2008 Bahadir Balban
 */
#ifndef __FS0_TASK_H__
#define __FS0_TASK_H__

#include <lib/idpool.h>
#include <l4/lib/list.h>
#include <l4/api/kip.h>

#define __TASKNAME__			__VFSNAME__

#define TCB_NO_SHARING				0
#define	TCB_SHARED_VM				(1 << 0)
#define	TCB_SHARED_FILES			(1 << 1)
#define TCB_SHARED_FS				(1 << 2)

#define TASK_FILES_MAX			32

struct task_fd_head {
	int fd[TASK_FILES_MAX];
	struct id_pool *fdpool;
	int tcb_refs;
};

struct task_fs_data {
	struct vnode *curdir;
	struct vnode *rootdir;
	int tcb_refs;
};

/* Thread control block, fs0 portion */
struct tcb {
	l4id_t tid;
	struct link list;
	unsigned long shpage_address;
	struct task_fd_head *files;
	struct task_fs_data *fs_data;
};

/* Structures used when receiving new task info from pager */
struct task_data {
	unsigned long tid;
	unsigned long shpage_address;
};

struct task_data_head {
	unsigned long total;
	struct task_data tdata[];
};

struct tcb *find_task(int tid);
int init_task_data(void);

#endif /* __FS0_TASK_H__ */
