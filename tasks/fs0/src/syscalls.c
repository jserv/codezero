/*
 * Some syscall stubs
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <l4/api/errno.h>
#include <l4lib/types.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <lib/malloc.h>
#include <string.h>
#include <stdio.h>
#include <task.h>
#include <vfs.h>

/*
 * This notifies mm0 that this is the fd that refers to this vnode number
 * from now on.
 *
 * MM0 *also* keeps track of fd's because mm0 is a better candidate
 * for handling syscalls that access file content (i.e. read/write) since
 * it maintains the page cache.
 */
int send_pager_opendata(l4id_t sender, int fd, unsigned long vnum)
{
	int err;

	write_mr(L4SYS_ARG0, sender);
	write_mr(L4SYS_ARG1, fd);
	write_mr(L4SYS_ARG2, vnum);

	if ((err = l4_send(PAGER_TID, L4_IPC_TAG_OPENDATA)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	return 0;
}

int do_open(l4id_t sender, const char *pathname, int flags, unsigned int mode)
{
	struct vnode *v;
	struct tcb *t;
	int fd;

	/* FIXME: Use strnlen */
	char *pathcopy = kmalloc(strlen(pathname));
	/* FIXME: Use strncpy */
	memcpy(pathcopy, pathname, strlen(pathname));

	/* Get the vnode */
	if (IS_ERR(v = vfs_lookup(vfs_root.pivot->sb, pathcopy)))
		return (int)v;

	/* Get the task */
	BUG_ON(!(t = find_task(sender)));

	/* Get a new fd */
	BUG_ON(!(fd = id_new(t->fdpool)));

	/* Assign the new fd with the vnode's number */
	t->fd[fd] = v->vnum;

	/* Tell mm0 about opened vnode information */
	BUG_ON(send_pager_opendata(sender, fd, v->vnum) < 0);

	return 0;
}

/* FIXME:
 * - Is it already open?
 * - Allocate a copy of path string since lookup destroys it
 * - Check flags and mode.
 */
int sys_open(l4id_t sender, const char *pathname, int flags, unsigned int mode)
{
	return do_open(sender, pathname, flags, mode);
}

int sys_read(l4id_t sender, int fd, void *buf, int count)
{
	return 0;
}

/*
 * FIXME: Read count is not considered yet. Take that into account.
 * FIXME: Do we pass this buf back to mm0. Do we get it from mm0???
 * Fix generic_vnode_lookup as well.
 */
int sys_readdir(l4id_t sender, int fd, void *buf, int count)
{
	struct vnode *v;
	struct tcb *t;

	/* Get the task */
	BUG_ON(!(t = find_task(sender)));

	/* Convert fd to vnode. */
	BUG_ON(!(v = t->fd[fd]));

	/* Ensure vnode is a directory */
	if (!vfs_isdir(v))
		return -ENOTDIR;

	/* Simply read its contents */
	v->ops.readdir(v, buf, count);

	return 0;
}

int sys_write(l4id_t sender, int fd, const void *buf, int count)
{
	return 0;
}

int sys_lseek(l4id_t sender, int fd, int offset, int whence)
{
	return 0;
}

