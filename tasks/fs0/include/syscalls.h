/*
 * System call function signatures.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#ifndef __FS0_SYSCALLS_H__
#define __FS0_SYSCALLS_H__

#include <task.h>

/* Posix calls */
int sys_open(struct tcb *sender, const char *pathname, int flags, u32 mode);
int sys_readdir(struct tcb *sender, int fd, void *buf, int count);
int sys_mkdir(struct tcb *sender, const char *pathname, unsigned int mode);
int sys_chdir(struct tcb *sender, const char *pathname);

/* Calls from pager that completes a posix call */

int pager_sys_open(struct tcb *sender, l4id_t opener, int fd);
int pager_sys_read(struct tcb *sender, unsigned long vnum, unsigned long f_offset,
		   unsigned long npages, void *pagebuf);

int pager_sys_write(struct tcb *sender, unsigned long vnum, unsigned long f_offset,
		   unsigned long npages, void *pagebuf);

int pager_sys_close(struct tcb *sender, l4id_t closer, int fd);
int pager_update_stats(struct tcb *sender, unsigned long vnum,
		       unsigned long newsize);

int pager_notify_fork(struct tcb *sender, l4id_t parid,
		      l4id_t chid, unsigned long utcb_address);

#endif /* __FS0_SYSCALLS_H__ */
