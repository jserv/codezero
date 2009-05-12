/*
 * Execve test.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <tests.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

extern char _start_test1[];
extern char _end_test1[];

int exectest(void)
{
	int fd;
	void *exec_start = (void *)_start_test1;
	unsigned long size = _end_test1 - _start_test1;
	int left, cnt;
	char *argv[5];

	/* First create a new file and write the executable data to that file */
	if ((fd = open("/home/bahadir/test1.axf", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
		test_printf("OPEN: %d\n", errno);
		goto out_err;
	}

	left = size;
	while (left != 0) {
		if ((cnt = write(fd, exec_start, left)) < 0)
			goto out_err;
		left -= cnt;
	}

	if (close(fd) < 0)
		goto out_err;

	/* Set up some arguments */
	argv[0] = "FIRST ARG";
	argv[1] = "SECOND ARG";
	argv[2] = "THIRD ARG";
	argv[3] = "FOURTH ARG";
	argv[4] = 0;

	/* Execute the file */
	execve("/home/bahadir/test1.axf", argv, 0);

out_err:
	printf("EXECVE TEST -- FAILED --\n");
	return 0;
}

