/*
 * Stack region helper routines.
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_STACK_H__
#define __LIB_STACK_H__

int stack_pool_init(unsigned long stack_start,
			unsigned long stack_end,
			unsigned long stack_size);

void *stack_new_space(int nitems, int size);
int stack_delete_space(void *stack_address, int nitems, int size);

#endif /* __LIB_STACK_H__ */
