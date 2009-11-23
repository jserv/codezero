/*
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __UTCB_H__
#define __UTCB_H__

#include <l4lib/types.h>
#include <l4lib/arch/utcb.h>

int utcb_init(void);

/* Bora start */
#include <l4lib/tcb.h>

/* Checks if l4_set_stack_params is called. */
#define IS_UTCB_SETUP()	(lib_utcb_range_size)

unsigned long get_utcb_addr(struct l4lib_tcb *task);
int delete_utcb_addr(struct l4lib_tcb *task);
/* Bora end */

#endif /* __UTCB_H__ */
