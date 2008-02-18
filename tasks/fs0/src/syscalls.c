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
#include <lib/pathstr.h>
#include <lib/malloc.h>
#include <string.h>
#include <stdio.h>
#include <task.h>
#include <stat.h>
#include <vfs.h>
#include <alloca.h>

/*
 * This notifies mm0 that this is the fd that refers to this vnode number
 * from now on.
 *
 * MM0 *also* keeps track of fd's because mm0 is a better candidate
 * for handling syscalls that access file content (i.e. read/write) since
 * it maintains the page cache.
 */
int send_pager_sys_open(l4id_t sender, int fd, unsigned long vnum, unsigned long size)
{
	int err;

	write_mr(L4SYS_ARG0, sender);
	write_mr(L4SYS_ARG1, fd);
	write_mr(L4SYS_ARG2, vnum);
	write_mr(L4SYS_ARG3, size);

	if ((err = l4_send(PAGER_TID, L4_IPC_TAG_PAGER_SYSOPEN)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	return 0;
}

/* Creates a node under a directory, e.g. a file, directory. */
int vfs_create(const char *pathname, unsigned int mode)
{
	char *pathbuf = alloca(strlen(pathname) + 1);
	char *parentpath = pathbuf;
	struct vnode *vparent;
	char *nodename;
	int err;

	strcpy(pathbuf, pathname);

	/* The last component is to be created */
	nodename = splitpath_end(&parentpath, '/');

	/* Check that the parent directory exists. */
	if (IS_ERR(vparent = vfs_lookup_bypath(vfs_root.pivot->sb, parentpath)))
		return (int)vparent;

	/* The parent vnode must be a directory. */
	if (!vfs_isdir(vparent))
		return -ENOENT;

	/* Create new directory under the parent */
	if ((err = vparent->ops.mknod(vparent, nodename, mode)) < 0)
		return err;

	return 0;
}

/* FIXME:
 * - Is it already open?
 * - Allocate a copy of path string since lookup destroys it
 * - Check flags and mode.
 */
int sys_open(l4id_t sender, const char *pathname, int flags, unsigned int mode)
{
	char *pathbuf = alloca(strlen(pathname) + 1);
	struct vnode *v;
	struct tcb *t;
	int fd;
	int err;

	strcpy(pathbuf, pathname);

	/* Get the vnode */
	if (IS_ERR(v = vfs_lookup_bypath(vfs_root.pivot->sb, pathbuf))) {
		if (!flags & O_CREAT) {
			return (int)v;
		} else {
			if ((err = vfs_create(pathname, mode)) < 0)
				return err;
		}
	}
	/* Get the task */
	BUG_ON(!(t = find_task(sender)));

	/* Get a new fd */
	BUG_ON(!(fd = id_new(t->fdpool)));

	/* Assign the new fd with the vnode's number */
	t->fd[fd] = v->vnum;

	/* Tell the pager about opened vnode information */
	BUG_ON(send_pager_sys_open(sender, fd, v->vnum, v->size) < 0);

	return 0;
}

int sys_mkdir(l4id_t sender, const char *pathname, unsigned int mode)
{
	return vfs_create(pathname, mode);
}

int sys_read(l4id_t sender, int fd, void *buf, int count)
{
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

/*
 * Reads @count bytes of posix struct dirents into @buf
 */
int sys_readdir(l4id_t sender, int fd, void *buf, int count)
{
	unsigned long vnum;
	struct vnode *v;
	struct tcb *t;
	unsigned long nbytes;
	int err;

	/* Get the task */
	BUG_ON(!(t = find_task(sender)));

	/* Convert fd to vnum. */
	BUG_ON(!(vnum = t->fd[fd]));

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL; /* No such vnode */

	/* Ensure vnode is a directory */
	if (!vfs_isdir(v))
		return -ENOTDIR;

	/* Read the whole content from fs, if haven't done so yet */
	if ((err = v->ops.readdir(v)) < 0)
		return err;

	/* Bytes to read, minimum of vnode size and count requested */
	nbytes = (v->size <= count) ? v->size : count;

	/* Do we have those bytes at hand? */
	if (v->dirbuf.buffer && (v->dirbuf.npages * PAGE_SIZE) >= nbytes) {
		memcpy(buf, v->dirbuf.buffer, nbytes);
		return nbytes;
	}

	return 0;
}

