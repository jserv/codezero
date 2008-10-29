/*
 * Copyright (C) 2007 Bahadir Balban
 *
 * MM0 Posix system call prototypes and structure
 * definitions for converting data in message registers
 * into system call argument format.
 */

#ifndef __MM0_SYSARGS_H__
#define __MM0_SYSARGS_H__

#include <sys/types.h>
#include <l4lib/types.h>
#include <task.h>

/* For reading argument data from a system call */
struct sys_mmap_args {
	void *start;
	size_t length;
	int prot;
	int flags;
	int fd;
	off_t offset;
};

void *sys_mmap(struct tcb *sender, void *start, size_t length, int prot,
	       int flags, int fd, off_t offset);

int sys_munmap(struct tcb *sender, void *vaddr, unsigned long size);
int sys_msync(struct tcb *task, void *start, unsigned long length, int flags);

struct sys_shmat_args {
	l4id_t shmid;
	const void *shmaddr;
	int shmflg;
};

void *sys_shmat(struct tcb *requester, l4id_t shmid, const void *shmaddr, int shmflg);
int sys_shmdt(struct tcb *requester, const void *shmaddr);

struct sys_shmget_args {
	key_t key;
	int size;
	int shmflg;
};

int sys_shmget(key_t key, int size, int shmflg);

int sys_fork(struct tcb *parent);
void sys_exit(struct tcb *task, int status);

#endif /* __MM0_SYSARGS_H__ */

