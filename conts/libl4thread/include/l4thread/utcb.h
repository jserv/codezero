/*
 * UTCB handling helper routines
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_UTCB_H__
#define __LIB_UTCB_H__

#include <tcb.h>

unsigned long get_utcb_addr(struct l4t_tcb *task);
int delete_utcb_addr(struct l4t_tcb *task);

#endif /* __LIB_UTCB_H__ */
