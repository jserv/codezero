/*
 * Stack space helper routines.
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_STACK_H__
#define __LIB_STACK_H__

/* Checks if l4_set_stack_params is called. */
#define IS_STACK_SETUP()	(lib_stack_size)

int stack_pool_init(unsigned long stack_start,
			unsigned long stack_end,
			unsigned long stack_size);

void *get_stack_space(void);
int delete_stack_space(void *stack_address);

#endif /* __LIB_STACK_H__ */
