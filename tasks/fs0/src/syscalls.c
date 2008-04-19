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
#include <path.h>

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

/* Directories only for now */
void print_vnode(struct vnode *v)
{
	struct dentry *d, *c;

	printf("Vnode names:\n");
	list_for_each_entry(d, &v->dentries, vref) {
		printf("%s\n", d->name);
		printf("Children dentries:\n");
		list_for_each_entry(c, &d->children, child)
			printf("%s\n", c->name);
	}
}

/* Creates a node under a directory, e.g. a file, directory. */
struct vnode *vfs_create(struct tcb *task, struct pathdata *pdata,
			 unsigned int mode)
{
	struct vnode *vparent, *newnode;
	const char *nodename;

	/* The last component is to be created */
	nodename = pathdata_last_component(pdata);

	/* Check that the parent directory exists. */
	if (IS_ERR(vparent = vfs_lookup_bypath(pdata)))
		return vparent;

	/* The parent vnode must be a directory. */
	if (!vfs_isdir(vparent))
		return PTR_ERR(-ENOENT);

	/* Create new directory under the parent */
	if (IS_ERR(newnode = vparent->ops.mknod(vparent, nodename, mode)))
		return newnode;

	// print_vnode(vparent);
	return newnode;
}

/* FIXME:
 * - Is it already open?
 * - Allocate a copy of path string since lookup destroys it
 * - Check flags and mode.
 *
 * TODO:
 * - All return paths should return by destroying pdata.
 * - Need another pdata for vfs_create since first lookup destroys it.
 * - Or perhaps we check O_CREAT first, and do lookup once, without the
 *   last path component which is to be created.
 */
int sys_open(l4id_t sender, const char *pathname, int flags, unsigned int mode)
{
	struct pathdata *pdata;
	struct vnode *v;
	struct tcb *task;
	int fd;
	int retval;

	/* Get the task */
	BUG_ON(!(task = find_task(sender)));

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task))) {
		l4_ipc_return((int)pdata);
		return 0;
	}

	/* Creating new file */
	if (flags & O_CREAT) {
		/* Make sure mode identifies a file */
		mode |= S_IFREG;
		if (IS_ERR(v = vfs_create(task, pdata, mode))) {
			retval = (int)v;
			goto out;
		}
	} else {
		/* Not creating, just opening, get the vnode */
		if (IS_ERR(v = vfs_lookup_bypath(pdata))) {
			retval = (int)v;
			goto out;
		}
	}

	/* Get a new fd */
	BUG_ON((fd = id_new(task->fdpool)) < 0);
	retval = fd;

	/* Assign the new fd with the vnode's number */
	task->fd[fd] = v->vnum;

	/* Tell the pager about opened vnode information */
	BUG_ON(pager_sys_open(sender, fd, v->vnum, v->size) < 0);

out:
	pathdata_destroy(pdata);
	l4_ipc_return(retval);
	return 0;
}

int sys_close(l4id_t sender, int fd)
{
	struct tcb *task;

	/* Get the task */
	BUG_ON(!(task = find_task(sender)));

	/* Validate file descriptor */
	if (fd < 0 || fd > TASK_OFILES_MAX) {
	       l4_ipc_return(-EBADF);
	       return 0;
	}
	if (!task->fd[fd]) {
		l4_ipc_return(-EBADF);
		return 0;
	}

	/* Finish I/O on file */

	return 0;
}

int sys_mkdir(l4id_t sender, const char *pathname, unsigned int mode)
{
	struct tcb *task;
	struct pathdata *pdata;
	struct vnode *v;

	/* Get the task */
	BUG_ON(!(task = find_task(sender)));

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task))) {
		l4_ipc_return((int)pdata);
		return 0;
	}

	/* Make sure we create a directory */
	mode |= S_IFDIR;

	/* Create the directory or fail */
	if (IS_ERR(v = vfs_create(task, pdata, mode)))
		l4_ipc_return((int)v);
	else
		l4_ipc_return(0);

	/* Destroy extracted path data */
	pathdata_destroy(pdata);
	return 0;
}

int sys_chdir(l4id_t sender, const char *pathname)
{
	struct vnode *v;
	struct tcb *task;
	struct pathdata *pdata;

	/* Get the task */
	BUG_ON(!(task = find_task(sender)));

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task))) {
		l4_ipc_return((int)pdata);
		return 0;
	}

	/* Get the vnode */
	if (IS_ERR(v = vfs_lookup_bypath(pdata))) {
		l4_ipc_return((int)v);
		return 0;
	}

	/* Ensure it's a directory */
	if (!vfs_isdir(v)) {
		l4_ipc_return(-ENOTDIR);
		return 0;
	}

	/* Assign the current directory pointer */
	task->curdir = v;

	/* Destroy extracted path data */
	pathdata_destroy(pdata);
	l4_ipc_return(0);
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

	if (fd < 0 || fd > TASK_OFILES_MAX) {
	       l4_ipc_return(-EBADF);
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

	fill_dirent(buf, v->vnum, nbytes, VFS_STR_CURDIR);
	nbytes += dirent_size;
	buf += dirent_size;
	count -= dirent_size;

	if (count < dirent_size)
		return 0;

	fill_dirent(buf, d->parent->vnode->vnum, nbytes, VFS_STR_PARDIR);
	nbytes += dirent_size;
	buf += dirent_size;
	count -= dirent_size;

	/* Copy fs-specific dir to buf in struct dirent format */
	if ((total = v->ops.filldir(buf, v, count)) < 0) {
		l4_ipc_return(total);
		return 0;
	}
	nbytes += total;
	l4_ipc_return(nbytes);

	return 0;
}

