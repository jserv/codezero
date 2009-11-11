/*
 * Stack region helper routines.
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_STACK_H__
#define __LIB_STACK_H__

/* Symbolic constants and macros */
#define IS_STACK_SETUP()	(lib_stack_size)

int stack_pool_init(unsigned long stack_start,
			unsigned long stack_end,
			unsigned long stack_size);

void *get_stack_space(int nitems, int size);
int delete_stack_space(void *stack_address, int nitems, int size);

#endif /* __LIB_STACK_H__ */
