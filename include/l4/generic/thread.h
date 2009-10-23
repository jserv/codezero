/*
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __GENERIC_THREAD_H__
#define __GENERIC_THREAD_H__

#include <l4/generic/tcb.h>

/* Thread id creation and deleting */
void thread_id_pool_init(void);
int thread_id_new(void);
int thread_id_del(int tid);

void task_destroy_current(void);
void task_make_zombie(struct ktcb *task);

#endif /* __GENERIC_THREAD_H__ */
