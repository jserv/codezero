/*
 * Thread control block.
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_TCB_H__
#define __LIB_TCB_H__

#include <l4/lib/list.h>

/* Keeps all the struct utcb_descs belonging to a thread group together. */
struct utcb_head {
	struct link list;
};

/* A simple thread control block for the thread library. */
struct tcb {
	/* Task list */
	struct link list;

	/* Task id */
	int tid;

	/* Chain of utcb descriptors */
	struct utcb_head *utcb_head;

	/* Stack and utcb address */
	unsigned long utcb_addr;
	unsigned long stack_addr;
};

/* All the threads handled by the thread lib are kept in this list. */
struct global_list {
	int total;
	struct link list;
};

struct tcb *find_task(int tid);
struct tcb *l4_tcb_alloc_init(struct tcb *parent, unsigned int flags);
void global_add_task(struct tcb *task);
void global_remove_task(struct tcb *task);

#endif /* __LIB_TCB_H__ */
