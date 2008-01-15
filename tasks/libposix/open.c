/*
 * l4/posix glue for open()
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <fcntl.h>

static inline int l4_open(const char *pathname, int flags, mode_t mode)
{
	int fd;

	write_mr(L4SYS_ARG0, (unsigned long)pathname);
	write_mr(L4SYS_ARG1, flags);
	write_mr(L4SYS_ARG2, (u32)mode);

	/* Call pager with shmget() request. Check ipc error. */
	if ((errno = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_OPEN)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	/* Check if syscall itself was successful */
	if ((fd = l4_get_retval()) < 0) {
		printf("%s: OPEN Error: %d.\n", __FUNCTION__, fd);
		errno = fd;
		return -1;

	}
	return fd;
}

int open(const char *pathname, int oflag, ...)
{
	mode_t mode = 0;

	if (oflag & O_CREAT) {
		va_list arg;
		va_start(arg, oflag);
		mode = va_arg(arg, mode_t);
		va_end(arg);
	}
	return l4_open(pathname, oflag, mode);
}

