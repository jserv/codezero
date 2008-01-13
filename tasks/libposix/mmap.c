/*
 * Glue logic between posix mmap/munmap functions
 * and their L4 implementation.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>

struct mmap_descriptor {
	void *start;
	size_t length;
	int prot;
	int flags;
	int fd;
	off_t offset;
};

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
	/* Not enough MRs for all arguments, therefore we fill in a structure */
	struct mmap_descriptor desc = {
		.start = start,
		.length = length,
		.prot = prot,
		.flags = flags,
		.fd = fd,
		.offset = offset,
	};
	int err;

	write_mr(L4SYS_ARG0, (unsigned long)&desc);

	/* Call pager with MMAP request. */
	if ((errno = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_MMAP)) < 0) {
		printf("%s: IPC Error: %d.\n", __FUNCTION__, errno);
		return MAP_FAILED;
	}
	/* Check if syscall itself was successful */
	if ((errno = l4_get_retval()) < 0) {
		printf("%s: MMAP Error: %d.\n", __FUNCTION__, errno);
		return MAP_FAILED;
	}
	return 0;
}

int munmap(void *start, size_t length)
{
	write_mr(L4SYS_ARG0, (unsigned long)start);
	write_mr(L4SYS_ARG1, length);

	/* Call pager with MMAP request. */
	if ((errno = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_MMAP)) < 0) {
		printf("%s: IPC Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	/* Check if syscall itself was successful */
	if ((errno = l4_get_retval()) < 0) {
		printf("%s: MUNMAP Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	return 0;
}

int l4_msync(void *start, size_t length, int flags)
{
	write_mr(L4SYS_ARG0, (unsigned long)start);
	write_mr(L4SYS_ARG1, length);
	write_mr(L4SYS_ARG2, flags);

	/* Call pager with MMAP request. */
	if ((errno = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_MSYNC)) < 0) {
		printf("%s: IPC Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	/* Check if syscall itself was successful */
	if ((errno = l4_get_retval()) < 0) {
		printf("%s: MUNMAP Error: %d.\n", __FUNCTION__, errno);
		return -1;
	}
	return 0;
}

int msync(void *start, size_t length, int flags)
{
	return l4_msync(start, length, flags);
}

