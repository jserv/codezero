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

void main(void)
{
	wait_pager(0);
	if (getpid() == 2) {
		printf("EXECVE TEST         -- PASSED --\n", getpid());
		printf("\nThread (%d): Continues to sync with the pager...\n\n", getpid());
		while (1)
			wait_pager(0);
	}
	_exit(0);
}

