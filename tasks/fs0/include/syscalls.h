/*
 * System call function signatures.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#ifndef __FS0_SYSCALLS_H__
#define __FS0_SYSCALLS_H__

int sys_open(l4id_t sender, char *pathname, int flags, u32 mode);
int sys_read(l4id_t sender, int fd, void *buf, int cnt);
int sys_write(l4id_t sender, int fd, void *buf, int cnt);
int sys_lseek(l4id_t sender, int fd, unsigned long offset, int whence);

#endif /* __FS0_SYSCALLS_H__ */
