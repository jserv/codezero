/*
 * l4/posix glue for fork()
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/utcb.h>
#include <l4/macros.h>
#include INC_GLUE(memory.h)

static inline int l4_fork(void)
{
	int err;

	/* Call pager with open() request. Check ipc error. */
	if ((err = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_FORK)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}
	/* Check if syscall itself was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: OPEN Error: %d.\n", __FUNCTION__, err);
		return err;
	}
	return err;
}

int fork(void)
{
	int ret = l4_fork();

	/* If error, return positive error code */
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	/* else return value */
	return ret;

}

