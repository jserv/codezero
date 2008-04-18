
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <tests.h>

int fileio(void)
{
	int fd;
	ssize_t cnt;
	int err;

	char *str = "I WROTE TO THIS FILE\n";

	if ((fd = open("/home/bahadir/newfile.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
		perror("OPEN");
		return 0;
	}

	if ((int)(cnt = write(fd, str, strlen(str))) < 0) {
		perror("WRITE");
		return 0;
	}

	if ((err = close(fd)) < 0)
		perror("CLOSE");

	return 0;
}

