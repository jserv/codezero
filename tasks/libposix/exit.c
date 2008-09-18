
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <unistd.h>
#include <l4/macros.h>

static inline void l4_exit(int status)
{
	int err;

	write_mr(L4SYS_ARG0, status);

	/* Call pager with exit() request. */
	err = l4_send(PAGER_TID, L4_IPC_TAG_EXIT);
	printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
	/* This call should not fail or return */
	BUG();
}

void exit(int status)
{
	l4_exit(status);
}
