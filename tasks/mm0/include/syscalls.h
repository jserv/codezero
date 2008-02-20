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

/* For reading argument data from a system call */
struct sys_mmap_args {
	void *start;
	size_t length;
	int prot;
	int flags;
	int fd;
	off_t offset;
};

int sys_mmap(l4id_t sender, void *start, size_t length, int prot,
	     int flags, int fd, off_t offset);

int sys_munmap(l4id_t sender, void *vaddr, unsigned long size);

struct sys_shmat_args {
	l4id_t shmid;
	const void *shmaddr;
	int shmflg;
};

void *sys_shmat(l4id_t requester, l4id_t shmid, const void *shmaddr, int shmflg);
int sys_shmdt(l4id_t requester, const void *shmaddr);

struct sys_shmget_args {
	key_t key;
	int size;
	int shmflg;
};

int sys_shmget(key_t key, int size, int shmflg);

#endif /* __MM0_SYSARGS_H__ */

