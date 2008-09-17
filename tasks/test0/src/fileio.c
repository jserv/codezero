
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <tests.h>
#include <l4lib/arch/syslib.h>

int fileio2(void)
{
	int fd;
	ssize_t cnt;
	int err;
	char buf[128];
	off_t offset;
	int tid = self_tid();

	char *str = "I WROTE TO THIS FILE\n";

	if ((fd = open("/home/bahadir/newfile2.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
		perror("OPEN");
		return -1;
	}
	printf("%d: Created newfile2.txt\n", tid);

	printf("%d: write.\n", tid);
	if ((int)(cnt = write(fd, str, strlen(str))) < 0) {
		perror("WRITE");
		return -1;
	}
	printf("%d: close.\n", tid);
	if ((err = close(fd)) < 0) {
		printf("Close failed.\n");
		perror("CLOSE");
		return -1;
	}
	printf("%d: re-open.\n", tid);
	if ((fd = open("/home/bahadir/newfile2.txt", O_RDWR, S_IRWXU)) < 0) {
		perror("OPEN");
		return -1;
	}

	printf("%d: lseek.\n", tid);
	if ((int)(offset = lseek(fd, 0, SEEK_SET)) < 0) {
		perror("LSEEK");
		return -1;
	}
	printf("%d: read.\n", tid);
	if ((int)(cnt = read(fd, buf, strlen(str))) < 0) {
		perror("READ");
		return -1;
	}

	printf("%d: Read: %d bytes from file.\n", tid, cnt);
	if (cnt) {
		printf("%d: Read string: %s\n", tid, buf);
		if (strcmp(buf, str))
			return -1;
	}

	printf("%d: close.\n", tid);
	if ((err = close(fd)) < 0) {
		perror("CLOSE");
		return -1;
	}

	return 0;
}

int fileio(void)
{
	int fd;
	ssize_t cnt;
	int err;
	char buf[128];
	off_t offset;
	int tid = self_tid();

	char *str = "I WROTE TO THIS FILE\n";

	if ((fd = open("/home/bahadir/newfile.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
		perror("OPEN");
		return -1;
	}
	printf("%d: Created newfile.txt\n", tid);

	printf("%d: write.\n", tid);
	if ((int)(cnt = write(fd, str, strlen(str))) < 0) {
		perror("WRITE");
		return -1;
	}

	printf("%d: lseek.\n", tid);
	if ((int)(offset = lseek(fd, 0, SEEK_SET)) < 0) {
		perror("LSEEK");
		return -1;
	}
	printf("%d: read.\n", tid);
	if ((int)(cnt = read(fd, buf, strlen(str))) < 0) {
		perror("READ");
		return -1;
	}

	printf("%d: Read: %d bytes from file.\n", tid, cnt);
	if (cnt) {
		printf("%d: Read string: %s\n", tid, buf);
		if (strcmp(buf, str))
			return -1;
	}

	printf("%d: close.\n", tid);
	if ((err = close(fd)) < 0) {
		perror("CLOSE");
		return -1;
	}

	return 0;
}

#if defined(HOST_TESTS)
int main(void)
{
	printf("File IO test 1:\n");
	if (fileio() == 0)
		printf("-- PASSED --\n");
	else
		printf("-- FAILED --\n");

	printf("File IO test 2:\n");
	if (fileio2() == 0)
		printf("-- PASSED --\n");
	else
		printf("-- FAILED --\n");


	return 0;
}
#endif

