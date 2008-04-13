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
int pager_sys_open(l4id_t sender, int fd, unsigned long vnum, unsigned long size)
{
	int err;

	write_mr(L4SYS_ARG0, sender);
	write_mr(L4SYS_ARG1, fd);
	write_mr(L4SYS_ARG2, vnum);
	write_mr(L4SYS_ARG3, size);

	if ((err = l4_send(PAGER_TID, L4_IPC_TAG_PAGER_OPEN)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	return 0;
}

/* Creates a node under a directory, e.g. a file, directory. */
int vfs_create(struct tcb *task, const char *pathname, unsigned int mode)
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
	if (IS_ERR(vparent = vfs_lookup_bypath(task, parentpath)))
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
	struct tcb *task;
	int fd;
	int err;

	strcpy(pathbuf, pathname);

	/* Get the task */
	BUG_ON(!(task = find_task(sender)));

	/* Get the vnode */
	if (IS_ERR(v = vfs_lookup_bypath(task, pathbuf))) {
		if (!(flags & O_CREAT)) {
			l4_ipc_return((int)v);
			return 0;
		} else {
			if ((err = vfs_create(task, pathname, mode)) < 0) {
				l4_ipc_return(err);
				return 0;
			}
		}
	}

	/* Get a new fd */
	BUG_ON((fd = id_new(task->fdpool)) < 0);

	/* Assign the new fd with the vnode's number */
	task->fd[fd] = v->vnum;

	/* Tell the pager about opened vnode information */
	BUG_ON(pager_sys_open(sender, fd, v->vnum, v->size) < 0);

	l4_ipc_return(0);
	return 0;
}

int sys_close(l4id_t sender, int fd)
{
	return 0;
}

int sys_mkdir(l4id_t sender, const char *pathname, unsigned int mode)
{
	struct tcb *task;

	/* Get the task */
	BUG_ON(!(task = find_task(sender)));

	return vfs_create(task, pathname, mode);
}

int sys_chdir(l4id_t sender, const char *pathname)
{
	char *pathbuf = alloca(strlen(pathname) + 1);
	struct vnode *v;
	struct tcb *task;

	strcpy(pathbuf, pathname);

	/* Get the task */
	BUG_ON(!(task = find_task(sender)));

	/* Get the vnode */
	if (IS_ERR(v = vfs_lookup_bypath(task, pathbuf)))
		return (int)v;

	/* Ensure it's a directory */
	if (!vfs_isdir(v))
		return -ENOTDIR;

	/* Assign the current directory pointer */
	task->curdir = v;

	return 0;
}

/*
 * Note this can be solely called by the pager and is not the posix read call.
 * That call is in the pager. This merely supplies the pages the pager needs
 * if they're not in the page cache.
 */
int pager_sys_read(l4id_t sender, unsigned long vnum, unsigned long f_offset,
		   unsigned long npages, void *pagebuf)
{
	struct vnode *v;
	int err;

	if (sender != PAGER_TID)
		return -EINVAL;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL; /* No such vnode */

	/* Ensure vnode is not a directory */
	if (vfs_isdir(v))
		return -EISDIR;

	if ((err = v->fops.read(v, f_offset, npages, pagebuf)) < 0)
		return err;

	return 0;
}

int pager_sys_write(l4id_t sender, unsigned long vnum, unsigned long f_offset,
		   unsigned long npages, void *pagebuf)
{
	struct vnode *v;
	int err;

	if (sender != PAGER_TID)
		return -EINVAL;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL; /* No such vnode */

	/* Ensure vnode is not a directory */
	if (vfs_isdir(v))
		return -EISDIR;

	if ((err = v->fops.write(v, f_offset, npages, pagebuf)) < 0)
		return err;

	return 0;
}

/*
 * FIXME: Here's how this should have been:
 * v->ops.readdir() -> Reads fs-specific directory contents. i.e. reads
 * the directory buffer, doesn't care however contained vnode details are
 * stored.
 *
 * After reading, it converts the fs-spceific contents into generic vfs
 * dentries and populates the dentries of those vnodes.
 *
 * If vfs_readdir() is issued, those generic dentries are converted into
 * the posix-defined directory record structure. During this on-the-fly
 * generation, pseudo-entries such as . and .. are also added.
 *
 * If this layering is not done, i.e. the low-level dentry buffer already
 * keeps this record structure and we try to return that, then we wont
 * have a chance to add the pseudo-entries . and .. These record entries
 * are essentially created from parent vnode and current vnode but using
 * the names . and ..
 */

int fill_dirent(void *buf, unsigned long vnum, int offset, char *name)
{
	struct dirent *d = buf;

	d->inum = (unsigned int)vnum;
	d->offset = offset;
	d->rlength = sizeof(struct dirent);
	strncpy((char *)d->name, name, DIRENT_NAME_MAX);

	return d->rlength;
}

/*
 * Reads @count bytes of posix struct dirents into @buf. This implements
 * the raw dirent read syscall upon which readdir() etc. posix calls
 * can be built in userspace.
 *
 * FIXME: Ensure buf is in shared utcb, and count does not exceed it.
 */
int sys_readdir(l4id_t sender, int fd, void *buf, int count)
{
	int dirent_size = sizeof(struct dirent);
	int total = 0, nbytes = 0;
	unsigned long vnum;
	struct vnode *v;
	struct dentry *d;
	struct tcb *t;

	/* Get the task */
	BUG_ON(!(t = find_task(sender)));

	/* Check address is in task's utcb */
	if ((unsigned long)buf < t->utcb_address ||
	    (unsigned long)buf > t->utcb_address + PAGE_SIZE) {
		l4_ipc_return(-EINVAL);
		return 0;
	}

	/* Convert fd to vnum. */
	BUG_ON((vnum = t->fd[fd]) < 0);

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum))) {
		l4_ipc_return(-EINVAL);
		return 0; /* No such vnode */
	}
	d = list_entry(v->dentries.next, struct dentry, vref);

	/* Ensure vnode is a directory */
	if (!vfs_isdir(v)) {
		l4_ipc_return(-ENOTDIR);
		return 0;
	}

	/* Write pseudo-entries . and .. to user buffer */
	if (count < dirent_size)
		return 0;

	printf("%s: Filling in . (curdir)\n", __TASKNAME__);
	fill_dirent(buf, v->vnum, nbytes, ".");
	nbytes += dirent_size;
	buf += dirent_size;
	count -= dirent_size;

	if (count < dirent_size)
		return 0;

	printf("%s: Filling in .. (pardir)\n", __TASKNAME__);
	fill_dirent(buf, d->parent->vnode->vnum, nbytes, "..");
	nbytes += dirent_size;
	buf += dirent_size;
	count -= dirent_size;

	printf("%s: Filling in other dir contents\n", __TASKNAME__);
	/* Copy fs-specific dir to buf in struct dirent format */
	if ((total = v->ops.filldir(buf, v, count)) < 0) {
		l4_ipc_return(total);
		return 0;
	}
	nbytes += total;
	l4_ipc_return(nbytes);

	return 0;
}

