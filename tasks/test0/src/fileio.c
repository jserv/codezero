
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <tests.h>
#include <errno.h>

int fileio(void)
{
	int fd;
	ssize_t cnt;
	int err;
	char buf[128];
	off_t offset;
	int tid = getpid();
	char *str = "I WROTE TO THIS FILE\n";
	char filename[128];

	memset(buf, 0, 128);
	memset(filename, 0, 128);
	sprintf(filename, "/home/bahadir/newfile%d.txt", tid);
	if ((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
		test_printf("OPEN: %d", errno);
		goto out_err;
	}
	test_printf("%d: Created %s\n", tid, filename);

	test_printf("%d: write.\n", tid);
	if ((int)(cnt = write(fd, str, strlen(str))) < 0) {
		test_printf("WRITE: %d", errno);
		goto out_err;
	}

	test_printf("%d: lseek.\n", tid);
	if ((int)(offset = lseek(fd, 0, SEEK_SET)) < 0) {
		test_printf("LSEEK: %d", errno);
		goto out_err;
	}
	test_printf("%d: read.\n", tid);
	if ((int)(cnt = read(fd, buf, strlen(str))) < 0) {
		test_printf("READ: %d", errno);
		goto out_err;
	}

	test_printf("%d: Read: %d bytes from file.\n", tid, cnt);
	if (cnt) {
		test_printf("%d: Read string: %s\n", tid, buf);
		if (strcmp(buf, str)) {
			test_printf("Strings not the same:\n");
			test_printf("Str: %s\n", str);
			test_printf("Buf: %s\n", buf);
			goto out_err;
		}
	}

	if ((err = close(fd)) < 0) {
		test_printf("CLOSE: %d", errno);
		goto out_err;
	}

	if (getpid() == parent_of_all)
		printf("FILE IO TEST   -- PASSED --\n");
	return 0;

out_err:
	printf("FILE IO TEST   -- FAILED --\n");
	return 0;
}

#if defined(HOST_TESTS)
int main(void)
{
	test_printf("File IO test 1:\n");
	if (fileio() == 0)
		test_printf("-- PASSED --\n");
	else
		test_printf("-- FAILED --\n");

	test_printf("File IO test 2:\n");
	if (fileio2() == 0)
		test_printf("-- PASSED --\n");
	else
		test_printf("-- FAILED --\n");


	return 0;
}
#endif

