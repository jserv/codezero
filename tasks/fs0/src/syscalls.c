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
#include <syscalls.h>

#define NILFD			-1

/*
 * This informs mm0 about an opened file descriptors.
 *
 * MM0 *also* keeps track of fd's because mm0 is a better candidate
 * for handling syscalls that access file content (i.e. read/write) since
 * it maintains the page cache. MM0 is not notified about opened files
 * but is rather informed when it asks to be. This avoids deadlocks by
 * keeping the request flow in one way.
 */

int pager_sys_open(struct tcb *pager, l4id_t opener, int fd)
{
	struct tcb *task;
	struct vnode *v;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);
	//
	if (pager->tid != PAGER_TID)
		return -EINVAL;

	/* Check if such task exists */
	if (!(task = find_task(opener)))
		return -ESRCH;

	/* Check if that fd has been opened */
	if (task->fd[fd] == NILFD)
		return -EBADF;

	/* Search the vnode by that vnum */
	if (IS_ERR(v = vfs_lookup_byvnum(vfs_root.pivot->sb,
					 task->fd[fd])))
		return (int)v;

	/*
	 * Write file information, they will
	 * be sent via the return reply.
	 */
	write_mr(L4SYS_ARG0, v->vnum);
	write_mr(L4SYS_ARG1, v->size);

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

/*
 * Pager notifies vfs about a closed file descriptor.
 *
 * FIXME: fsync + close could be done under a single "close" ipc
 * from pager. Currently there are 2 ipcs: 1 fsync + 1 fd close.
 */
int pager_sys_close(struct tcb *sender, l4id_t closer, int fd)
{
	struct tcb *task;
	int err;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	BUG_ON(!(task = find_task(closer)));

	if ((err = id_del(task->fdpool, fd)) < 0) {
		printf("%s: Error releasing fd identifier.\n",
		       __FUNCTION__);
		return err;
	}
	task->fd[fd] = NILFD;

	return 0;
}

/* FIXME:
 * - Is it already open?
 * - Allocate a copy of path string since lookup destroys it
 * - Check flags and mode.
 */
int sys_open(struct tcb *task, const char *pathname, int flags, unsigned int mode)
{
	struct pathdata *pdata;
	struct vnode *v;
	int fd;
	int retval;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task)))
		return (int)pdata;

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

out:
	pathdata_destroy(pdata);
	return retval;
}

int sys_mkdir(struct tcb *task, const char *pathname, unsigned int mode)
{
	struct pathdata *pdata;
	struct vnode *v;
	int ret = 0;

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task)))
		return (int)pdata;

	/* Make sure we create a directory */
	mode |= S_IFDIR;

	/* Create the directory or fail */
	if (IS_ERR(v = vfs_create(task, pdata, mode)))
		ret = (int)v;

	/* Destroy extracted path data */
	pathdata_destroy(pdata);
	return ret;
}

int sys_chdir(struct tcb *task, const char *pathname)
{
	struct vnode *v;
	struct pathdata *pdata;
	int ret = 0;

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task)))
		return (int)pdata;

	/* Get the vnode */
	if (IS_ERR(v = vfs_lookup_bypath(pdata))) {
		ret = (int)v;
		goto out;
	}

	/* Ensure it's a directory */
	if (!vfs_isdir(v)) {
		ret = -ENOTDIR;
		goto out;
	}

	/* Assign the current directory pointer */
	task->curdir = v;

out:
	/* Destroy extracted path data */
	pathdata_destroy(pdata);
	return ret;
}

void fill_kstat(struct vnode *v, struct kstat *ks)
{
	ks->vnum = (u64)v->vnum;
	ks->mode = v->mode;
	ks->links = v->links;
	ks->uid = v->owner & 0xFFFF;
	ks->gid = (v->owner >> 16) & 0xFFFF;
	ks->size = v->size;
	ks->blksize = v->sb->blocksize;
	ks->atime = v->atime;
	ks->mtime = v->mtime;
	ks->ctime = v->ctime;
}

int sys_fstat(struct tcb *task, int fd, void *statbuf)
{
	struct vnode *v;
	unsigned long vnum;

	/* Get the vnum */
	if (fd < 0 || fd > TASK_FILES_MAX || task->fd[fd] == NILFD)
		return -EBADF;

	vnum = task->fd[fd];

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	/* Fill in the c0-style stat structure */
	fill_kstat(v, statbuf);

	return 0;
}

/*
 * Returns codezero-style stat structure which in turn is
 * converted to posix style stat structure via the libposix
 * library in userspace.
 */
int sys_stat(struct tcb *task, const char *pathname, void *statbuf)
{
	struct vnode *v;
	struct pathdata *pdata;
	int ret = 0;

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task)))
		return (int)pdata;

	/* Get the vnode */
	if (IS_ERR(v = vfs_lookup_bypath(pdata))) {
		ret = (int)v;
		goto out;
	}

	/* Fill in the c0-style stat structure */
	fill_kstat(v, statbuf);

out:
	/* Destroy extracted path data */
	pathdata_destroy(pdata);
	return ret;
}

/*
 * Note this can be solely called by the pager and is not the posix read call.
 * That call is in the pager. This merely supplies the pages the pager needs
 * if they're not in the page cache.
 */
int pager_sys_read(struct tcb *pager, unsigned long vnum, unsigned long f_offset,
		   unsigned long npages, void *pagebuf)
{
	struct vnode *v;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	if (pager->tid != PAGER_TID)
		return -EINVAL;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	/* Ensure vnode is not a directory */
	if (vfs_isdir(v))
		return -EISDIR;

	return v->fops.read(v, f_offset, npages, pagebuf);
}

int pager_update_stats(struct tcb *pager, unsigned long vnum,
		       unsigned long newsize)
{
	struct vnode *v;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	if (pager->tid != PAGER_TID)
		return -EINVAL;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	v->size = newsize;
	v->sb->ops->write_vnode(v->sb, v);

	return 0;
}

/*
 * This can be solely called by the pager and is not the posix write call.
 * That call is in the pager. This writes the dirty pages of a file
 * back to disk via vfs.
 *
 * The buffer must be contiguous by page, if npages > 1.
 */
int pager_sys_write(struct tcb *pager, unsigned long vnum, unsigned long f_offset,
		    unsigned long npages, void *pagebuf)
{
	struct vnode *v;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	if (pager->tid != PAGER_TID)
		return -EINVAL;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	/* Ensure vnode is not a directory */
	if (vfs_isdir(v))
		return -EISDIR;

	printf("%s/%s: Writing to vnode %lu, at pgoff 0x%x, %d pages, buf at 0x%x\n",
		__TASKNAME__, __FUNCTION__, vnum, f_offset, npages, pagebuf);

	/*
	 * If the file is extended, write silently extends it.
	 * But we expect an explicit pager_update_stats from the
	 * pager to update the new file size on the vnode.
	 */
	return v->fops.write(v, f_offset, npages, pagebuf);
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
int sys_readdir(struct tcb *t, int fd, void *buf, int count)
{
	int dirent_size = sizeof(struct dirent);
	int total = 0, nbytes = 0;
	unsigned long vnum;
	struct vnode *v;
	struct dentry *d;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	/* Check address is in task's utcb */
	if ((unsigned long)buf < t->utcb_address ||
	    (unsigned long)buf > t->utcb_address + PAGE_SIZE)
		return -EINVAL;

	if (fd < 0 || fd > TASK_FILES_MAX || t->fd[fd] == NILFD)
		return -EBADF;

	vnum = t->fd[fd];

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	d = list_entry(v->dentries.next, struct dentry, vref);

	/* Ensure vnode is a directory */
	if (!vfs_isdir(v))
		return -ENOTDIR;

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
	if ((total = v->ops.filldir(buf, v, count)) < 0)
		return total;

	return nbytes + total;
}

