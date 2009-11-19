/*
 * UTCB handling helper routines.
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_UTCB_H__
#define __LIB_UTCB_H__

#include <tcb.h>

/* Checks if l4_set_stack_params is called. */
#define IS_UTCB_SETUP()	(lib_utcb_range_size)

unsigned long get_utcb_addr(struct tcb *task);
int delete_utcb_addr(struct tcb *task);

#endif /* __LIB_UTCB_H__ */
