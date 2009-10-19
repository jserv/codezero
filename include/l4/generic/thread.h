/*
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __THREAD_H__
#define __THREAD_H__

#include <l4/generic/tcb.h>

/* Thread id creation and deleting */
void thread_id_pool_init(void);
int thread_id_new(void);
int thread_id_del(int tid);

void thread_destroy_current(void);
int thread_destroy(struct task_ids *ids);
void thread_make_zombie(struct ktcb *task);

#endif /* __THREAD_H__ */
