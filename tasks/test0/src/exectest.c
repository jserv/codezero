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
	printf("%s: Creating new executable file.\n", __FUNCTION__);
	if ((fd = open("/home/bahadir/test1.axf", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
		perror("OPEN");
		return -1;
	}

	printf("%s: Writing to the executable file.\n", __FUNCTION__);
	left = size;
	while (left != 0) {
		cnt = write(fd, exec_start, left);
		if (cnt < 0) {
			printf("Error writing to file.\n");
			return -1;
		}
		left -= cnt;
	}

	close(fd);

	/* Set up some arguments */
	argv[0] = "FIRST ARG";
	argv[1] = "SECOND ARG";
	argv[2] = "THIRD ARG";
	argv[3] = "FOURTH ARG";
	argv[4] = 0;

	printf("%s: Executing the file.\n", __FUNCTION__);
	/* Execute the file */
	execve("/home/bahadir/test1.axf", argv, 0);

	return 0;
}

