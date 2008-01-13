/*
 * l4/posix glue for lseek()
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

static inline off_t l4_lseek(int fildes, off_t offset, int whence)
{
	off_t offres;

	write_mr(L4SYS_ARG0, fildes);
	write_mr(L4SYS_ARG1, offset);
	write_mr(L4SYS_ARG2, whence);

	/* Call pager with shmget() request. Check ipc error. */
	if ((errno = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_LSEEK)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	/* Check if syscall itself was successful */
	if ((offres = l4_get_retval()) < 0) {
		printf("%s: OPEN Error: %d.\n", __FUNCTION__, (int)offres);
		errno = (int)offres;
		return -1;

	}
	return offres;
}

off_t lseek(int fildes, off_t offset, int whence)
{
	return l4_lseek(fildes, offset, whence);
}

