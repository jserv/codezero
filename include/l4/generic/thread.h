/*
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __THREAD_H__
#define __THREAD_H__


/* Thread id creation and deleting */
void thread_id_pool_init(void);
int thread_id_new(void);
int thread_id_del(int tid);


#endif /* __THREAD_H__ */
