/*
 * Fork test.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <tests.h>

int global = 0;
extern pid_t pid;

int forktest(void)
{
	pid_t myid;

	/* 16 forks */
	for (int i = 0; i < 4; i++)
		fork();

	myid = getpid();
	pid = myid;
	if (global != 0) {
		printf("Global not zero.\n");
		printf("-- FAILED --\n");
		goto out;
	}
	global = myid;

	if (global != myid) {
		printf("Global has not changed to myid.\n");
		printf("-- FAILED --\n");
		goto out;
	}

	/* Print only when failed, otherwise too many pass messages */
	printf("PID: %d, my global: %d\n", myid, global);
	printf("-- PASSED --\n");
out:
	printf("PID: %d exiting...\n", myid);
	_exit(0);
}

