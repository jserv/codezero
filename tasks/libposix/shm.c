/*
 * This is the glue logic between posix shared memory functions
 * and their L4 implementation.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <errno.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>


static inline int l4_shmget(l4id_t key, int size, int shmflg)
{
	write_mr(L4SYS_ARG0, key);
	write_mr(L4SYS_ARG1, size);
	write_mr(L4SYS_ARG2, shmflg);

	/* Call pager with shmget() request. Check ipc error. */
	if ((errno = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_SHMGET)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	/* Check if syscall itself was successful */
	if ((errno = l4_get_retval()) < 0) {
		printf("%s: SHMGET Error: %d.\n", __FUNCTION__, errno);
		return -1;

	}
	/* Obtain shmid. */
	return read_mr(L4SYS_ARG0);
}

static inline void *l4_shmat(l4id_t shmid, const void *shmaddr, int shmflg)
{
	write_mr(L4SYS_ARG0, shmid);
	write_mr(L4SYS_ARG1, (unsigned long)shmaddr);
	write_mr(L4SYS_ARG2, shmflg);

	/* Call pager with shmget() request. Check ipc error. */
	if ((errno = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_SHMAT)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, errno);
		return (void *)-1;
	}
	/* Check if syscall itself was successful */
	if ((errno = l4_get_retval()) < 0) {
		printf("%s: SHMAT Error: %d.\n", __FUNCTION__, errno);
		return (void *)-1;

	}
	/* Obtain shm base. */
	return (void *)read_mr(L4SYS_ARG0);
}

static inline int l4_shmdt(const void *shmaddr)
{
	write_mr(L4SYS_ARG0, (unsigned long)shmaddr);

	/* Call pager with shmget() request. Check ipc error. */
	if ((errno = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_SHMDT)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	/* Check if syscall itself was successful */
	if ((errno = l4_get_retval()) < 0) {
		printf("%s: SHMDT Error: %d.\n", __FUNCTION__, errno);
		return -1;

	}
	return 0;
}

int shmget(key_t key, size_t size, int shmflg)
{
	return l4_shmget(key, size, shmflg);
}

void *shmat(int shmid, const void *shmaddr, int shmflg)
{
	return l4_shmat(shmid, shmaddr, shmflg);
}

int shmdt(const void *shmaddr)
{
	return l4_shmdt(shmaddr);
}


