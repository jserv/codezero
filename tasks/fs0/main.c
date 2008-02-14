/*
 * FS0. Filesystem implementation
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/kip.h>
#include <l4lib/utcb.h>
#include <l4lib/ipcdefs.h>
#include <fs.h>
#include <init.h>
#include <kdata.h>
#include <syscalls.h>
#include <task.h>

/*
 * TODO:
 * - Have a dentry cache searchable by name
 * - Have a vnode cache searchable by vnum (and name?)
 * - fs-specific readdir would read contents by page range, and add vnodes/dentries
 *   to their caches, while populating the directory vnode being read.
 * - Have 2 vfs_lookup() flavors, one that searches a path, one that searches a vnum.
 * - dirbuf is either allocated by low-level readdir, or else by a higher level, i.e.
 *   either high-level vfs code, or the mm0 page cache.
 * - readdir provides a posix-compliant dirent structure list in dirbuf.
 * - memfs dentries should be identical to posix struct dirents.
 */

/* Synchronise with pager via a `wait' tagged ipc with destination as pager */
void wait_pager(l4id_t partner)
{
	l4_send(partner, L4_IPC_TAG_WAIT);
	printf("%s: Pager synced with us.\n", __TASKNAME__);
}

void handle_fs_requests(void)
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
	case L4_IPC_TAG_OPEN:
		sys_open(sender, (void *)mr[0], (int)mr[1], (u32)mr[2]);
		break;
	case L4_IPC_TAG_READ:
		sys_read(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;
	case L4_IPC_TAG_WRITE:
		sys_write(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;
	case L4_IPC_TAG_LSEEK:
		sys_lseek(sender, (int)mr[0], (int)mr[1], (int)mr[2]);
		break;
	default:
		printf("%s: Unrecognised ipc tag (%d)"
		       "received. Ignoring.\n", __TASKNAME__, mr[MR_TAG]);
	}
}

void main(void)
{
	printf("\n%s: Started with tid: %d\n", __TASKNAME__, self_tid());

	initialise();

	wait_pager(PAGER_TID);

	while (1) {
		handle_fs_requests();
	}
}

