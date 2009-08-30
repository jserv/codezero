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
#include <posix/errno.h>

extern char _start_test_exec[];
extern char _end_test_exec[];

int exectest(void)
{
	int fd, err;
	void *exec_start = (void *)_start_test_exec;
	unsigned long size = _end_test_exec - _start_test_exec;
	int left, cnt;
	char *argv[5];
	char filename[128];

	memset(filename, 0, 128);
	sprintf(filename, "/home/bahadir/execfile%d", getpid());

	/* First create a new file and write the executable data to that file */
	if ((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
		err = errno;
		test_printf("OPEN: %d, for %s\n", errno, filename);
		/* If it is a minor error like EEXIST, create one with different name */
		if (errno == EEXIST) {
			sprintf(filename, "/home/bahadir/execfile%d-2",
				getpid());
			if ((fd = open(filename,
				       O_RDWR | O_CREAT | O_TRUNC, S_IRWXU))
				       < 0) {
				printf("OPEN: %d, failed twice, "
				       "last time for %s\n",
				       errno, filename);
				goto out_err;
			}
		} else
			goto out_err;
	}

	left = size;
	test_printf("Writing %d bytes to %s\n", left, filename);
	while (left != 0) {
		if ((cnt = write(fd, exec_start, left)) < 0)
			goto out_err;
		left -= cnt;
	}

	if ((err = close(fd)) < 0) {
		goto out_err;
	}

	/* Set up some arguments */
	argv[0] = "FIRST ARG";
	argv[1] = "SECOND ARG";
	argv[2] = "THIRD ARG";
	argv[3] = "FOURTH ARG";
	argv[4] = 0;

	/* Execute the file */
	err = execve(filename, argv, 0);

out_err:
	printf("EXECVE failed with %d\n", err);
	printf("EXECVE TEST      -- FAILED --\n");
	return 0;
}

