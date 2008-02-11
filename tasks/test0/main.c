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

#define __TASKNAME__			"test0"

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
	printf("\n%s: Started with tid %d.\n", __TASKNAME__, self_tid());
	/* Sync with pager */
	while (1)
		wait_pager(0);

#if 0
	/* Check mmap/munmap */
	mmaptest();

	/* Check shmget/shmat/shmdt */
	shmtest();
#endif
}

