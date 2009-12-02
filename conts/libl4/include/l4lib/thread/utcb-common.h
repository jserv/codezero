/*
 * UTCB handling common helper routines.
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __UTCB_COMMON_H__
#define __UTCB_COMMON_H__

#include <l4/lib/list.h>
#include <l4lib/thread/tcb.h>

struct l4lib_utcb_desc {
	struct link list;
	unsigned long utcb_base;
	struct id_pool *slots;
};

int utcb_pool_init(unsigned long utcb_start, unsigned long utcb_end);

unsigned long utcb_new_slot(struct l4lib_utcb_desc *desc);
int utcb_delete_slot(struct l4lib_utcb_desc *desc, unsigned long address);

struct l4lib_utcb_desc *utcb_new_desc(void);
int utcb_delete_desc(struct l4lib_utcb_desc *desc);


/* Checks if l4_set_stack_params is called. */
#define IS_UTCB_SETUP()	(lib_utcb_range_size)

unsigned long get_utcb_addr(struct l4lib_tcb *task);
int delete_utcb_addr(struct l4lib_tcb *task);

#endif /* __UTCB_COMMON_H__ */
