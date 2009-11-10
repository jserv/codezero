/*
 * Thread creation userspace helpers
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_THREAD_H__
#define __LIB_THREAD_H__

int set_stack_params(unsigned long stack_top,
			unsigned long stack_bottom,
			unsigned long stack_size);
int set_utcb_params(unsigned long utcb_start, unsigned long utcb_end);

int l4thread_create(struct task_ids *ids, unsigned int flags,
			int (*func)(void *), void *arg);
void l4thread_kill(struct task_ids *ids);

#endif /* __LIB_THREAD_H__ */
