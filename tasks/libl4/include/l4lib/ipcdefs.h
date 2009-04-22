/*
 * Copyright (C) 2007, 2008 Bahadir Balban
 *
 * This file contains ipc definitions that are needed for server tasks
 * to communicate with each other. For example common shared memory ids
 * between two servers, or common ipc tags used between two servers are
 * defined here.
 */
#ifndef __IPCDEFS_H__
#define __IPCDEFS_H__

#include <l4/api/ipc.h>

/* SHMID used betweeen FS0 and BLKDEV0 servers */
#define FS_BLKDEV_SHMID			0

/*** IPC Tags used between server tasks ***/

/* For ping ponging */
#define L4_IPC_TAG_SYNC			3

/* To obtain default shared page address */
#define L4_IPC_TAG_SHPAGE		4

/* XXX: unused */
#define L4_IPC_TAG_GRANT		5

/* Posix system call tags */
#define L4_IPC_TAG_SHMGET		6
#define L4_IPC_TAG_SHMAT		7
#define L4_IPC_TAG_SHMDT		8
#define L4_IPC_TAG_MMAP			9
#define L4_IPC_TAG_MUNMAP		10
#define L4_IPC_TAG_MSYNC		11
#define L4_IPC_TAG_OPEN			12
#define L4_IPC_TAG_READ			13
#define L4_IPC_TAG_WRITE		14
#define L4_IPC_TAG_LSEEK		15
#define L4_IPC_TAG_CLOSE		16
#define L4_IPC_TAG_BRK			17
#define L4_IPC_TAG_READDIR		18
#define L4_IPC_TAG_MKDIR		19
#define L4_IPC_TAG_EXECVE		20
#define L4_IPC_TAG_CHDIR		21
#define L4_IPC_TAG_FORK			22
#define L4_IPC_TAG_STAT			23
#define L4_IPC_TAG_FSTAT		24
#define L4_IPC_TAG_FSYNC		25
#define L4_IPC_TAG_CLONE		26
#define L4_IPC_TAG_EXIT			27
#define L4_IPC_TAG_WAIT			28

/* Tags for ipc between fs0 and mm0 */
#define L4_IPC_TAG_TASKDATA		40
#define L4_IPC_TAG_PAGER_OPEN		41	/* vfs sends the pager open file data. */
#define L4_IPC_TAG_PAGER_READ		42	/* Pager reads file contents from vfs */
#define L4_IPC_TAG_PAGER_WRITE		43	/* Pager writes file contents to vfs */
#define L4_IPC_TAG_PAGER_CLOSE		44	/* Pager notifies vfs of file close */
#define L4_IPC_TAG_PAGER_UPDATE_STATS	45	/* Pager updates file stats in vfs */
#define L4_IPC_TAG_NOTIFY_FORK		46	/* Pager notifies vfs of process fork */
#define L4_IPC_TAG_NOTIFY_EXIT		47	/* Pager notifies vfs of process exit */
#define L4_IPC_TAG_PAGER_OPEN_BYPATH	48	/* Pager opens a vfs file by pathname */
#endif /* __IPCDEFS_H__ */
