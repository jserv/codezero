/*
 * Copyright (C) 2008 Bahadir Balban
 */
#ifndef __FS0_TASK_H__
#define __FS0_TASK_H__

#include <lib/idpool.h>
#include <l4/lib/list.h>

#define __TASKNAME__			"FS0"

#define TASK_OFILES_MAX			32

/* Thread control block, fs0 portion */
struct tcb {
	l4id_t tid;
	struct list_head list;
	int fd[TASK_OFILES_MAX];
	struct id_pool *fdpool;
};

struct tcb *find_task(int tid);
void init_task_data(void);

#endif /* __FS0_TASK_H__ */
