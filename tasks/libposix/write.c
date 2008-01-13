/*
 * l4/posix glue for write()
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

static inline int l4_write(int fd, const void *buf, size_t count)
{
	size_t wrcnt;

	write_mr(L4SYS_ARG0, fd);
	write_mr(L4SYS_ARG1, (const unsigned long)buf);
	write_mr(L4SYS_ARG2, count);

	/* Call pager with shmget() request. Check ipc error. */
	if ((errno = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_WRITE)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	/* Check if syscall itself was successful */
	if ((wrcnt = l4_get_retval()) < 0) {
		printf("%s: WRITE Error: %d.\n", __FUNCTION__, (int)wrcnt);
		errno = (int)wrcnt;
		return -1;

	}
	return wrcnt;
}

ssize_t write(int fd, const void *buf, size_t count)
{
	if (!count)
		return 0;

	return l4_write(fd, buf, count);
}

