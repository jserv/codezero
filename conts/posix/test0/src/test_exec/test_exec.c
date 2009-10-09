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
#include <stdlib.h>

void wait_pager(l4id_t partner)
{
	// printf("%s: Syncing with pager.\n", __TASKNAME__);
	for (int i = 0; i < 6; i++)
		write_mr(i, i);
	l4_send(partner, L4_IPC_TAG_SYNC);
	// printf("Pager synced with us.\n");
}

int main(int argc, char *argv[])
{
	wait_pager(0);
	char *parent_of_all;
	char pidbuf[10];


	/* Convert current pid to string */
	sprintf(pidbuf, "%d", getpid());

	if (strcmp(argv[0], "FIRST ARG") ||
	    strcmp(argv[1], "SECOND ARG") ||
	    strcmp(argv[2], "THIRD ARG") ||
	    strcmp(argv[3], "FOURTH ARG")) {
		printf("EXECVE TEST         -- FAILED --\n");
		goto out;
	}

	/* Get parent of all pid as a string from environment */
	parent_of_all = getenv("parent_of_all");

	/* Compare two pid strings. We use strings because we dont have atoi() */
	if (!strcmp(pidbuf, parent_of_all)) {
		printf("EXECVE TEST         -- PASSED --\n");
		printf("\nThread (%d): Continues to sync with the pager...\n\n", getpid());
		while (1)
			wait_pager(0);
	}
out:
	_exit(0);
}

