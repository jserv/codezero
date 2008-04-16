/*
 * l4/posix glue for mkdir()
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/utcb.h>
#include <l4/macros.h>
#include INC_GLUE(memory.h)

static inline int l4_chdir(const char *pathname)
{
	int fd;

	// write_mr(L4SYS_ARG0, (unsigned long)pathname);
	copy_to_utcb((void *)pathname, 0, strlen(pathname) + 1);
	write_mr(L4SYS_ARG0, (unsigned long)utcb_page);

	/* Call pager with shmget() request. Check ipc error. */
	if ((fd = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_CHDIR)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, fd);
		return fd;
	}
	/* Check if syscall itself was successful */
	if ((fd = l4_get_retval()) < 0) {
		printf("%s: MKDIR Error: %d.\n", __FUNCTION__, fd);
		return fd;
	}
	return fd;
}

int chdir(const char *pathname)
{
	int ret;

	/* If error, return positive error code */
	if ((ret = l4_chdir(pathname)) < 0) {
		errno = -ret;
		return -1;
	}
	/* else return value */
	return ret;
}

