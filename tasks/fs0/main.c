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
#include <posix/sys/time.h>
#include <l4/api/errno.h>

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
 *
 *   ALL DONE!!! But untested.
 *
 * - Add mkdir
 * - Add create
 * - Add read/write -> This will need page cache and mm0 involvement.
 *
 *   Done those, too. but untested.
 */

/* Synchronise with pager via a `wait' tagged ipc with destination as pager */
void wait_pager(l4id_t partner)
{
	l4_send(partner, L4_IPC_TAG_SYNC);
	// printf("%s: Pager synced with us.\n", __TASKNAME__);
}

void handle_fs_requests(void)
{
	u32 mr[MR_UNUSED_TOTAL];
	l4id_t senderid;
	struct tcb *sender;
	int ret;
	u32 tag;

	if ((ret = l4_receive(L4_ANYTHREAD)) < 0) {
		printf("%s: %s: IPC Error: %d. Quitting...\n", __TASKNAME__,
		       __FUNCTION__, ret);
		BUG();
	}

	/* Read conventional ipc data */
	tag = l4_get_tag();
	senderid = l4_get_sender();

	if (!(sender = find_task(senderid))) {
		l4_ipc_return(-ESRCH);
		return;
	}

	/* Read mrs not used by syslib */
	for (int i = 0; i < MR_UNUSED_TOTAL; i++)
		mr[i] = read_mr(MR_UNUSED_START + i);

	/* FIXME: Fix all these syscalls to read any buffer data from the caller task's utcb.
	 * Make sure to return -EINVAL if data is not valid. */
	switch(tag) {
	case L4_IPC_TAG_SYNC:
		printf("%s: Synced with waiting thread.\n", __TASKNAME__);
		return; /* No reply for this tag */
	case L4_IPC_TAG_OPEN:
		ret = sys_open(sender, (void *)mr[0], (int)mr[1], (unsigned int)mr[2]);
		break;
	case L4_IPC_TAG_MKDIR:
		ret = sys_mkdir(sender, (const char *)mr[0], (unsigned int)mr[1]);
		break;
	case L4_IPC_TAG_CHDIR:
		ret = sys_chdir(sender, (const char *)mr[0]);
		break;
	case L4_IPC_TAG_READDIR:
		ret = sys_readdir(sender, (int)mr[0], (void *)mr[1], (int)mr[2]);
		break;
	case L4_IPC_TAG_PAGER_READ:
		ret = pager_sys_read(sender, (unsigned long)mr[0], (unsigned long)mr[1],
				     (unsigned long)mr[2], (void *)mr[3]);
		break;
	case L4_IPC_TAG_PAGER_OPEN:
		ret = pager_sys_open(sender, (l4id_t)mr[0], (int)mr[1]);
		break;
	case L4_IPC_TAG_PAGER_OPEN_BYPATH:
		ret = pager_open_bypath(sender, (char *)mr[0]);
		break;
	case L4_IPC_TAG_PAGER_WRITE:
		ret = pager_sys_write(sender, (unsigned long)mr[0], (unsigned long)mr[1],
				      (unsigned long)mr[2], (void *)mr[3]);
		break;
	case L4_IPC_TAG_PAGER_CLOSE:
		ret = pager_sys_close(sender, (l4id_t)mr[0], (int)mr[1]);
		break;
	case L4_IPC_TAG_PAGER_UPDATE_STATS:
		ret = pager_update_stats(sender, (unsigned long)mr[0],
					 (unsigned long)mr[1]);
		break;
	case L4_IPC_TAG_NOTIFY_FORK:
		ret = pager_notify_fork(sender, (l4id_t)mr[0], (l4id_t)mr[1],
					(unsigned long)mr[2], (unsigned int)mr[3]);
		break;
	case L4_IPC_TAG_NOTIFY_EXIT:
		ret = pager_notify_exit(sender, (l4id_t)mr[0]);
		break;

	default:
		printf("%s: Unrecognised ipc tag (%d) "
		       "received from tid: %d. Ignoring.\n", __TASKNAME__,
		       mr[MR_TAG], sender);
	}

	/* Reply */
	if ((ret = l4_ipc_return(ret)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, ret);
		BUG();
	}
}

void main(void)
{
	printf("\n%s: Started with thread id %d\n", __TASKNAME__, self_tid());

	initialise();

	wait_pager(PAGER_TID);

	printf("%s: VFS service initialized. Listening requests.\n", __TASKNAME__);
	while (1) {
		handle_fs_requests();
	}
}

