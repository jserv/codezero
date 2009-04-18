/*
 * Some tests for posix syscalls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/kip.h>
#include <l4lib/utcb.h>
#include <l4lib/ipcdefs.h>
#include <tests.h>
#include <unistd.h>
#include <sys/types.h>

void wait_pager(l4id_t partner)
{
	// printf("%s: Syncing with pager.\n", __TASKNAME__);
	for (int i = 0; i < 6; i++)
		write_mr(i, i);
	l4_send(partner, L4_IPC_TAG_SYNC);
	// printf("Pager synced with us.\n");
}

pid_t pid;

void main(void)
{

	printf("\n%s: Started with tid %d.\n", __TASKNAME__, self_tid());
	/* Sync with pager */
	wait_pager(0);

	dirtest();

	// exectest();

	/* Check mmap/munmap */
	mmaptest();

	printf("Forking...\n");
	if ((pid = fork()) < 0)
		printf("Error forking...\n");

	if (pid == 0) {
		pid = getpid();
		printf("Child: file IO test 1.\n");
		if (fileio() == 0)
			printf("-- Fileio PASSED --\n");
		else
			printf("-- Fileio FAILED --\n");

		printf("Child: forktest.\n");
		if (forktest() == 0)
			printf("-- Fork PASSED -- \n");
		else
			printf("-- Fork FAILED -- \n");
	} else {
		printf("Parent: file IO test 2. child pid %d:\n", pid);
		if (fileio2() == 0)
			printf("-- Fileio2 PASSED --\n");
		else
			printf("-- Fileio2 FAILED --\n");
	}

	printf("Testing clone syscall...\n");
	if ((pid = fork()) < 0)
		printf("Error forking...\n");
	/* Child does the clonetest(). All of them will exit */
	if (pid == 0)
		clonetest();

	while (1)
		wait_pager(0);
#if 0
	/* Check shmget/shmat/shmdt */
	shmtest();
#endif
}

