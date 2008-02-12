/*
 * l4/posix glue for read() / sys_readdir()
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/os/posix/readdir.h>

static inline int l4_readdir(int fd, void *buf, size_t count)
{
	size_t cnt;

	write_mr(L4SYS_ARG0, fd);
	write_mr(L4SYS_ARG1, (unsigned long)buf);
	write_mr(L4SYS_ARG2, count);

	/* Call pager with shmget() request. Check ipc error. */
	if ((cnt = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_READDIR)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, cnt);
		return cnt;
	}
	/* Check if syscall itself was successful */
	if ((cnt = l4_get_retval()) < 0) {
		printf("%s: READ Error: %d.\n", __FUNCTION__, (int)cnt);
		return cnt;

	}
	return cnt;
}

static inline int l4_read(int fd, void *buf, size_t count)
{
	size_t cnt;

	write_mr(L4SYS_ARG0, fd);
	write_mr(L4SYS_ARG1, (unsigned long)buf);
	write_mr(L4SYS_ARG2, count);

	/* Call pager with shmget() request. Check ipc error. */
	if ((cnt = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_READ)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, cnt);
		return cnt;
	}
	/* Check if syscall itself was successful */
	if ((cnt = l4_get_retval()) < 0) {
		printf("%s: READ Error: %d.\n", __FUNCTION__, (int)cnt);
		return cnt;

	}
	return cnt;
}

ssize_t read(int fd, void *buf, size_t count)
{
	int ret;

	if (!count)
		return 0;

	ret = l4_read(fd, buf, count);

	/* If error, return positive error code */
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	/* else return value */
	return ret;

}

ssize_t os_readdir(int fd, void *buf, size_t count)
{
	int ret;

	if (!count)
		return 0;

	ret = l4_readdir(fd, buf, count);

	/* If error, return positive error code */
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	/* else return value */
	return ret;
}

