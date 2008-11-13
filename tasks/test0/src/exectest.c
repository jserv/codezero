/*
 * Execve test.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <tests.h>

void exectest(void)
{
	execve("/usr/home/bahadir/executable", 0, 0);
}

