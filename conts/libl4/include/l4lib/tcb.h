/*
 * Thread control block.
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_TCB_H__
#define __LIB_TCB_H__

#include <l4/lib/list.h>

/* Keeps all the struct utcb_descs belonging to a thread group together. */
struct l4lib_utcb_head {
	struct link list;
};

/* A simple thread control block for the thread library. */
struct l4lib_tcb {
	/* Task list */
	struct link list;

	/* Task id */
	int tid;

	/* Chain of utcb descriptors */
	struct l4lib_utcb_head *utcb_head;

	/* Stack and utcb address */
	unsigned long utcb_addr;
	unsigned long stack_addr;
};

/* All the threads handled by the thread lib are kept in this list. */
struct l4lib_global_list {
	int total;
	struct link list;
};

struct l4lib_tcb *l4lib_find_task(int tid);
struct l4lib_tcb *l4_tcb_alloc_init(struct l4lib_tcb *parent, unsigned int flags);
void l4lib_l4lib_global_add_task(struct l4lib_tcb *task);
void l4lib_global_remove_task(struct l4lib_tcb *task);

#endif /* __LIB_TCB_H__ */
