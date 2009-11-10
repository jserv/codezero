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
struct l4t_tcb {
	struct link list;
	int tid;
	struct utcb_head *utcb_head;
	unsigned long utcb_addr;
};

/* This struct keeps track of all the threads handled by the thread lib. */
struct l4t_global_list {
	int total;
	struct link list;
};

struct l4t_tcb *l4t_find_task(int tid);
struct l4t_tcb *l4t_tcb_alloc_init(struct l4t_tcb *parent, unsigned int flags);
void l4t_global_add_task(struct l4t_tcb *task);
void l4t_global_remove_task(struct l4t_tcb *task);

#endif /* __LIB_TCB_H__ */
