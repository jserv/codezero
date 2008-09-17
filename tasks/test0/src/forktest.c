/*
 * Fork test.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <tests.h>

int global = 0;

int forktest(void)
{
	pid_t myid;

	for (int i = 0; i < 4; i++)
		fork();

	myid = getpid();
	if (global != 0) {
		printf("-- FAILED --\n");
		goto out;
	}
	global = myid;

	if (global != myid) {
		printf("-- FAILED --\n");
		goto out;
	}

	/* Print only when failed, otherwise too many pass messages */
	//	printf("PID: %d, my global: %d\n", myid, global);
	// printf("-- PASSED --\n");
out:
	while(1)
		;
}

