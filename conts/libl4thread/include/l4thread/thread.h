/*
 * Thread creation userspace helpers
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_THREAD_H__
#define __LIB_THREAD_H__

#define START_ADDR(addr)	((unsigned long)(addr))
#define END_ADDR(addr)		((unsigned long)((addr) + (sizeof(addr))))

#define STACK_TOP_ADDR(stack)		(START_ADDR(stack))
#define STACK_BOTTOM_ADDR(stack)	(END_ADDR(stack))

#define UTCB_START_ADDR(utcb)	(START_ADDR(utcb))
#define UTCB_END_ADDR(utcb)	(END_ADDR(utcb))

int set_stack_params(unsigned long stack_top_addr,
			unsigned long stack_bottom_addr,
			unsigned long stack_size);
int set_utcb_params(unsigned long utcb_start_addr,
			unsigned long utcb_end_addr);

int thread_create(struct task_ids *ids, unsigned int flags,
			int (*func)(void *), void *arg);

#endif /* __LIB_THREAD_H__ */
