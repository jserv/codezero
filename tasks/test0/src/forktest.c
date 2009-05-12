/*
 * Fork test.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <tests.h>
#include <l4/macros.h>

int global = 0;

static pid_t pid;

int forktest(void)
{
	pid_t myid;
	pid_t parent = getpid();

	/* 16 forks */
	for (int i = 0; i < 4; i++)
		if (fork() < 0)
			goto out_err;

	myid = getpid();
	pid = myid;
	if (global != 0) {
		test_printf("Global not zero.\n");
		test_printf("-- FAILED --\n");
		goto out_err;
	}
	global += myid;

	if (global != myid)
		goto out_err;


	if (getpid() != parent) {
		/* Successful childs return here */
		_exit(0);
		BUG();
	}

	/* Parent of all comes here if successful */
	printf("FORK TEST      -- PASSED --\n");
	return 0;

	/* Any erroneous child or parent comes here */
out_err:
	printf("FORK TEST      -- FAILED --\n");
	return 0;
}

