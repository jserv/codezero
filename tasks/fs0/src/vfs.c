
#include <stdio.h>
#include <l4/api/ipc.h>
#include <l4lib/kip.h>
#include <l4lib/arch/syscalls.h>

/* Simply calls a wait ipc to sync with the given partner thread */
void wait_task(l4id_t partner)
{
	u32 tag = L4_IPC_TAG_WAIT;
	l4_ipc(partner, l4_nilthread, tag);
	printf("%d synced with us.\n", (int)partner);
}

void mount_root(void)
{
	/*
	 * We know the primary block device from compile-time.
	 * It is expected to have the root filesystem.
	 */
	wait_task(BLKDEV_TID);

	/* Set up a shared memory area with the block device */
}

