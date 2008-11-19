/*
 * Definitions for do_exit() flags
 *
 * Copyright (C) 2008 Bahadir Balban
 */

#ifndef __EXIT_H__
#define __EXIT_H__

#define EXIT_THREAD_DESTROY		(1 << 0)
#define EXIT_UNMAP_ALL_SPACE		(1 << 1)


void do_exit(struct tcb *task, unsigned int flags, int status);
#endif /* __EXIT_H__ */
