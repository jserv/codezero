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

pid_t parent_of_all;

void main(void)
{

	printf("\n%s: Started with thread id %d\n", __TASKNAME__, getpid());

	parent_of_all = getpid();

	wait_pager(0);


	printf("\n%s: Running POSIX API tests.\n", __TASKNAME__);

	dirtest();

	mmaptest();

	shmtest();

	forktest();

	fileio();

	clonetest();

	if (parent_of_all == getpid()) {
		ipc_full_test();
		ipc_extended_test();
	}
//	if (parent_of_all == getpid()) {
//		user_mutex_test();
//	}
	exectest();

	while (1)
		wait_pager(0);
}

