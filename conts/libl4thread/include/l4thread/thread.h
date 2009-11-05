/*
 * Thread creation userspace helpers
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_THREAD_H__
#define __LIB_THREAD_H__

#include "utcb.h"

#define STACK_TOP_ADDR(stack)	((unsigned long)(stack))
#define STACK_BOTTOM_ADDR(stack) \
			((unsigned long)((stack) + (sizeof(stack))))

int set_stack_params(unsigned long stack_top_addr,
			unsigned long stack_bottom_addr,
			unsigned long stack_size);
int thread_create(struct task_ids *ids, unsigned int flags,
			int (*func)(void *), void *arg);

#endif /* __LIB_THREAD_H__ */
