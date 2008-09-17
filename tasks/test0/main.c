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
	l4_send(partner, L4_IPC_TAG_WAIT);
	// printf("Pager synced with us.\n");
}

void main(void)
{
	pid_t pid;

	printf("\n%s: Started with tid %d.\n", __TASKNAME__, self_tid());
	/* Sync with pager */
	wait_pager(0);

	dirtest();

	printf("Forking...\n");

	if ((pid = fork()) < 0)
		printf("Error forking...\n");

	if (pid == 0) {
		printf("Child: file IO test 1.\n");
		if (fileio() == 0)
			printf("-- PASSED --\n");
		else
			printf("-- FAILED --\n");

		printf("Child: forktest.\n");
		if (forktest() == 0)
			printf("-- PASSED -- \n");
		else
			printf("-- FAILED -- \n");
	} else {
		printf("Parent: file IO test 2. child pid %d:\n", pid);
		if (fileio2() == 0)
			printf("-- PASSED --\n");
		else
			printf("-- FAILED --\n");
	}
	while (1)
		wait_pager(0);
#if 0
	/* Check mmap/munmap */
	mmaptest();

	/* Check shmget/shmat/shmdt */
	shmtest();
#endif
}

