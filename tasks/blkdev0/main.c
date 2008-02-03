/*
 * High-level block device handling.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <stdio.h>
#include <blkdev.h>

/*
 * Handles block device requests from fs0 using a combination of
 * server-specific and posix shm semantics
 */
void handle_block_device_request()
{
	u32 mr[MR_UNUSED_TOTAL];
	l4id_t sender;
	int err;
	u32 tag;

	printf("%s: Listening requests.\n", __TASKNAME__);

	if ((err = l4_receive(L4_ANYTHREAD)) < 0) {
		printf("%s: %s: IPC Error: %d. Quitting...\n", __TASKNAME__,
		       __FUNCTION__, err);
		BUG();
	}

	/* Read conventional ipc data */
	tag = l4_get_tag();
	sender = l4_get_sender();

	/* Read mrs not used by syslib */
	for (int i = 0; i < MR_UNUSED_TOTAL; i++)
		mr[i] = read_mr(i);

	switch(tag) {
	case L4_IPC_TAG_WAIT:
		printf("%s: Synced with waiting thread.\n", __TASKNAME__);
		break;
	case L4_IPC_TAG_BLOCK_OPEN:
		sys_open(sender, (void *)mr[0], (int)mr[1], (u32)mr[2]);
		break;
	default:
		printf("%s: Unrecognised ipc tag (%d)"
		       "received. Ignoring.\n", __TASKNAME__, mr[MR_TAG]);
	}
}

int main(void)
{
	/* Initialise the block devices */
	init_blkdev();

	while (1) {
		handle_block_device_request();
	}
	return 0;
}

