/*
 * System call function signatures.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#ifndef __FS0_SYSCALLS_H__
#define __FS0_SYSCALLS_H__

/* Posix calls */
int sys_open(l4id_t sender, char *pathname, int flags, u32 mode);
int sys_readdir(l4id_t sender, int fd, void *buf, int count);
int sys_mkdir(l4id_t sender, const char *pathname, unsigned int mode);
int sys_chdir(l4id_t sender, const char *pathname);

/* Calls from pager that completes a posix call */

int pager_sys_open(l4id_t sender, l4id_t opener, int fd);
int pager_sys_read(l4id_t sender, unsigned long vnum, unsigned long f_offset,
		   unsigned long npages, void *pagebuf);

int pager_sys_write(l4id_t sender, unsigned long vnum, unsigned long f_offset,
		   unsigned long npages, void *pagebuf);

int pager_sys_close(l4id_t sender, l4id_t closer, int fd);
int pager_update_stats(l4id_t sender, unsigned long vnum,
		       unsigned long newsize);

int pager_notify_fork(l4id_t sender, l4id_t parid,
		      l4id_t chid, unsigned long utcb_address);

#endif /* __FS0_SYSCALLS_H__ */
