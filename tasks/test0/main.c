/*
 * Some tests for posix syscalls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <l4lib/arch/message.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/kip.h>
#include <l4lib/utcb.h>
#include <l4/api/ipc.h>
#include <tests.h>

#define __TASKNAME__			"test0"

void wait_pager(l4id_t partner)
{
	u32 tag = L4_IPC_TAG_WAIT;
	printf("%s: Syncing with pager.\n", __TASKNAME__);
	l4_send(partner, tag);
	printf("Pager synced with us.\n");
}

void main(void)
{
	/* Sync with pager */
	wait_pager(0);

	/* Check mmap/munmap */
	mmaptest();

	/* Check shmget/shmat/shmdt */
	shmtest();
}

