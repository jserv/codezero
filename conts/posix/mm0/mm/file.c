/*
 * File read, write, open and close.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <init.h>
#include <vm_area.h>
#include <lib/malloc.h>
#include <mm/alloc_page.h>
#include <l4/macros.h>
#include <l4/api/errno.h>
#include <l4lib/types.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/kip.h>
#include <posix/sys/types.h>
#include <string.h>
#include <globals.h>
#include <file.h>
#include <user.h>
#include <test.h>

#include <lib/pathstr.h>
#include <lib/malloc.h>
#include <stdio.h>
#include <task.h>
#include <stat.h>
#include <vfs.h>
#include <alloca.h>
#include <path.h>
#include <syscalls.h>

/* Copy from one page's buffer into another page */
int page_copy(struct page *dst, struct page *src,
	      unsigned long dst_offset, unsigned long src_offset,
	      unsigned long size)
{
	void *dstvaddr, *srcvaddr;

	BUG_ON(dst_offset + size > PAGE_SIZE);
	BUG_ON(src_offset + size > PAGE_SIZE);

	dstvaddr = l4_map_helper((void *)page_to_phys(dst), 1);
	srcvaddr = l4_map_helper((void *)page_to_phys(src), 1);

#if 0
	printf("%s: Copying from page with offset %d to page with offset %d\n"
	       "src copy offset: 0x%x, dst copy offset: 0x%x, copy size: %d\n",
	       __FUNCTION__, src->offset, dst->offset, src_offset, dst_offset,
	       size);

	printf("%s: Copying string: %s\n", __FUNCTION__,
	       srcvaddr + src_offset);
#endif

	memcpy(dstvaddr + dst_offset, srcvaddr + src_offset, size);

	l4_unmap_helper(dstvaddr, 1);
	l4_unmap_helper(srcvaddr, 1);

	return 0;
}

int vfs_read(unsigned long vnum, unsigned long file_offset,
	     unsigned long npages, void *pagebuf)
{
	struct vnode *v;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	/* Ensure vnode is not a directory */
	if (vfs_isdir(v))
		return -EISDIR;

	return v->fops.read(v, file_offset, npages, pagebuf);
}

/* Directories only for now */
void print_vnode(struct vnode *v)
{
	struct dentry *d, *c;

	printf("Vnode names:\n");
	list_foreach_struct(d, &v->dentries, vref) {
		printf("%s\n", d->name);
		printf("Children dentries:\n");
		list_foreach_struct(c, &d->children, child)
			printf("%s\n", c->name);
	}
}



/*
 * Different from vfs_open(), which validates an already opened
 * file descriptor, this call opens a new vfs file by the pager
 * using the given path. The vnum handle and file length is returned
 * since the pager uses this information to access file pages.
 */
int vfs_open_bypath(const char *pathname, unsigned long *vnum, unsigned long *length)
{
	struct pathdata *pdata;
	struct tcb *task;
	struct vnode *v;
	int retval;

	/* Parse path data */
	if (IS_ERR(pdata = pathdata_parse(pathname,
					  alloca(strlen(pathname) + 1),
					  task)))
		return (int)pdata;

	/* Search the vnode by that path */
	if (IS_ERR(v = vfs_lookup_bypath(pdata))) {
		retval = (int)v;
		goto out;
	}

	*vnum = v->vnum;
	*length = v->size;

	return 0;

out:
	pathdata_destroy(pdata);
	return retval;
}

/*
 * When a task does a read/write/mmap request on a file, if
 * the file descriptor is unknown to the pager, this call
 * asks vfs if that file has been opened, and any other
 * relevant information.
 */
int vfs_open(l4id_t opener, int fd, unsigned long *vnum, unsigned long *length)
{
	struct tcb *task;
	struct vnode *v;

	/* Check if such task exists */
	if (!(task = find_task(opener)))
		return -ESRCH;

	/* Check if that fd has been opened */
	if (!task->files->fd[fd].vnum)
		return -EBADF;

	/* Search the vnode by that vnum */
	if (IS_ERR(v = vfs_lookup_byvnum(vfs_root.pivot->sb,
					 task->files->fd[fd].vnum)))
		return (int)v;

	/* Read file information */
	*vnum = v->vnum;
	*length = v->size;

	return 0;
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
	BUG_ON((fd = id_new(task->files->fdpool)) < 0);
	retval = fd;

	/* Why assign just vnum? Why not vmfile, vnode etc? */
	BUG();

	/* Assign the new fd with the vnode's number */
	task->files->fd[fd].vnum = v->vnum;

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
	task->fs_data->curdir = v;

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
	if (fd < 0 || fd > TASK_FILES_MAX || !task->files->fd[fd].vnum)
		return -EBADF;

	BUG(); /* Could just return vmfile->vnode here. */
	vnum = task->files->fd[fd].vnum;

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
 * Initialise a new file and the descriptor for it from given file data.
 * Could be called by an actual task or a pager
 */
struct vm_file *do_open2(struct tcb *task, int fd, unsigned long vnum, unsigned long length)
{
	struct vm_file *vmfile;

	/* Is this an open by a task (as opposed to by the pager)? */
	if (task) {
		/* fd slot must be empty */
		BUG_ON(task->files->fd[fd].vnum != 0);
		BUG_ON(task->files->fd[fd].cursor != 0);

		/* Assign vnum to given fd on the task */
		task->files->fd[fd].vnum = vnum;
		task->files->fd[fd].cursor = 0;
	}

	/* Check if that vm_file is already in the list */
	list_foreach_struct(vmfile, &global_vm_files.list, list) {

		/* Check whether it is a vfs file and if so vnums match. */
		if ((vmfile->type & VM_FILE_VFS) &&
		    vm_file_to_vnum(vmfile) == vnum) {

			/* Task opener? */
			if (task)
				/* Add a reference to it from the task */
				task->files->fd[fd].vmfile = vmfile;

			vmfile->openers++;
			return vmfile;
		}
	}

	/* Otherwise allocate a new one for this vnode */
	if (IS_ERR(vmfile = vfs_file_create()))
		return vmfile;

	/* Initialise and add a reference to it from the task */
	vm_file_to_vnum(vmfile) = vnum;
	vmfile->length = length;
	vmfile->vm_obj.pager = &file_pager;

	/* Task opener? */
	if (task)
		task->files->fd[fd].vmfile = vmfile;
	vmfile->openers++;

	/* Add to file list */
	global_add_vm_file(vmfile);

	return vmfile;
}

/* Initialise a new file and the descriptor for it from given file data */
int do_open(struct tcb *task, int fd, unsigned long vnum, unsigned long length)
{
	struct vm_file *vmfile;

	/* fd slot must be empty */
	BUG_ON(task->files->fd[fd].vnum != 0);
	BUG_ON(task->files->fd[fd].cursor != 0);

	/* Assign vnum to given fd on the task */
	task->files->fd[fd].vnum = vnum;
	task->files->fd[fd].cursor = 0;

	/* Check if that vm_file is already in the list */
	list_foreach_struct(vmfile, &global_vm_files.list, list) {

		/* Check whether it is a vfs file and if so vnums match. */
		if ((vmfile->type & VM_FILE_VFS) &&
		    vm_file_to_vnum(vmfile) == vnum) {

			/* Add a reference to it from the task */
			task->files->fd[fd].vmfile = vmfile;
			vmfile->openers++;
			return 0;
		}
	}

	/* Otherwise allocate a new one for this vnode */
	if (IS_ERR(vmfile = vfs_file_create()))
		return (int)vmfile;

	/* Initialise and add a reference to it from the task */
	vm_file_to_vnum(vmfile) = vnum;
	vmfile->length = length;
	vmfile->vm_obj.pager = &file_pager;
	task->files->fd[fd].vmfile = vmfile;
	vmfile->openers++;

	/* Add to file list */
	global_add_vm_file(vmfile);

	return 0;
}

int file_open(struct tcb *opener, int fd)
{
	int err;
	unsigned long vnum;
	unsigned long length;

	if (fd < 0 || fd > TASK_FILES_MAX)
		return -EINVAL;

	/* Ask vfs if such a file has been recently opened */
	if ((err = vfs_open(opener->tid, fd, &vnum, &length)) < 0)
		return err;

	/* Initialise local structures with received file data */
	if ((err = do_open(opener, fd, vnum, length)) < 0)
		return err;

	return 0;
}


/*
 * Inserts the page to vmfile's list in order of page frame offset.
 * We use an ordered list instead of a better data structure for now.
 */
int insert_page_olist(struct page *this, struct vm_object *vmo)
{
	struct page *before, *after;

	/* Add if list is empty */
	if (list_empty(&vmo->page_cache)) {
		list_insert_tail(&this->list, &vmo->page_cache);
		return 0;
	}

	/* Else find the right interval */
	list_foreach_struct(before, &vmo->page_cache, list) {
		after = link_to_struct(before->list.next, struct page, list);

		/* If there's only one in list */
		if (before->list.next == &vmo->page_cache) {
			/* Add as next if greater */
			if (this->offset > before->offset)
				list_insert(&this->list, &before->list);
			/* Add  as previous if smaller */
			else if (this->offset < before->offset)
				list_insert_tail(&this->list, &before->list);
			else
				BUG();
			return 0;
		}

		/* If this page is in-between two other, insert it there */
		if (before->offset < this->offset &&
		    after->offset > this->offset) {
			list_insert(&this->list, &before->list);
			return 0;
		}
		BUG_ON(this->offset == before->offset);
		BUG_ON(this->offset == after->offset);
	}
	BUG();
}

/*
 * This reads-in a range of pages from a file and populates the page cache
 * just like a page fault, but its not in the page fault path.
 */
int read_file_pages(struct vm_file *vmfile, unsigned long pfn_start,
		    unsigned long pfn_end)
{
	struct page *page;

	for (int f_offset = pfn_start; f_offset < pfn_end; f_offset++) {
		page = vmfile->vm_obj.pager->ops.page_in(&vmfile->vm_obj,
							 f_offset);
		if (IS_ERR(page)) {
			printf("%s: %s:Could not read page %d "
			       "from file with vnum: 0x%lu\n", __TASKNAME__,
			       __FUNCTION__, f_offset, vm_file_to_vnum(vmfile));
			return (int)page;
		}
	}

	return 0;
}

/*
 * The buffer must be contiguous by page, if npages > 1.
 */
int vfs_write(unsigned long vnum, unsigned long file_offset,
	      unsigned long npages, void *pagebuf)
{
	struct vnode *v;
	int fwrite_end;
	int ret;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	/* Ensure vnode is not a directory */
	if (vfs_isdir(v))
		return -EISDIR;

	//printf("%s/%s: Writing to vnode %lu, at pgoff 0x%x, %d pages, buf at 0x%x\n",
	//	__TASKNAME__, __FUNCTION__, vnum, f_offset, npages, pagebuf);

	if ((ret = v->fops.write(v, file_offset, npages, pagebuf)) < 0)
		return ret;

	/*
	 * If the file is extended, write silently extends it.
	 * We update the extended size here. Otherwise subsequent write's
	 * may fail by relying on wrong file size.
	 */
	fwrite_end = __pfn_to_addr(file_offset) + ret;
	if (v->size < fwrite_end) {
		v->size = fwrite_end;
		v->sb->ops->write_vnode(v->sb, v);
	}

	return ret;
}



/* Writes updated file stats back to vfs. (e.g. new file size) */
int vfs_update_file_stats(struct vm_file *f)
{
	unsigned long vnum = vm_file_to_vnum(f);
	struct vnode *v;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	v->size = f->length;
	v->sb->ops->write_vnode(v->sb, v);

	return 0;
}

/* Writes pages in cache back to their file */
int write_file_pages(struct vm_file *f, unsigned long pfn_start,
		     unsigned long pfn_end)
{
	int err;

	/* We have only thought of vfs files for this */
	BUG_ON(f->type != VM_FILE_VFS);

	/* Need not flush files that haven't been written */
	if (!(f->vm_obj.flags & VM_DIRTY))
		return 0;

	BUG_ON(pfn_end != __pfn(page_align_up(f->length)));
	for (int f_offset = pfn_start; f_offset < pfn_end; f_offset++) {
		err = f->vm_obj.pager->ops.page_out(&f->vm_obj, f_offset);
		if (err < 0) {
			printf("%s: %s:Could not write page %d "
			       "to file with vnum: 0x%lu\n", __TASKNAME__,
			       __FUNCTION__, f_offset, vm_file_to_vnum(f));
			return err;
		}
	}

	return 0;
}

/* Flush all dirty file pages and update file stats */
int flush_file_pages(struct vm_file *f)
{
	int err;

	if ((err = write_file_pages(f, 0, __pfn(page_align_up(f->length)))) < 0)
		return err;

	if ((err = vfs_update_file_stats(f)) < 0)
		return err;

	return 0;
}

/* Given a task and fd, syncs all IO on it */
int fsync_common(struct tcb *task, int fd)
{
	int err;

	/* Check fd validity */
	if (fd < 0 || fd > TASK_FILES_MAX)
		return -EINVAL;

	/*
	 * If we don't know about the file, even if it was
	 * opened by the vfs, it is sure that there's no
	 * pending IO on it. We simply return.
	 */
	if (!task->files->fd[fd].vmfile)
		return 0;

	/* Finish I/O on file */
	if ((err = flush_file_pages(task->files->fd[fd].vmfile)) < 0)
		return err;

	return 0;
}

void vm_file_put(struct vm_file *file)
{
	/* Reduce file's opener count */
	if (!(file->openers--))
		/* No openers left, check any mappers */
		if (!file->vm_obj.nlinks)
			/* No links or openers, delete the file */
			vm_file_delete(file);

}

/*
 * FIXME: fsync + close could be done under a single "close" ipc
 * from pager. Currently there are 2 ipcs: 1 fsync + 1 fd close.
 */

/* Closes the file descriptor and notifies vfs */
int do_close(struct tcb *task, int fd)
{
	int err;

	 printf("%s: Closing fd: %d on task %d\n", __FUNCTION__,
	       fd, task->tid);

	if ((err = id_del(task->files->fdpool, fd)) < 0) {
		printf("%s: Error releasing fd identifier.\n",
		       __FUNCTION__);
		return err;
	}

	/*
	 * If there was no IO on it, we may not know the file,
	 * we simply return here. Since we notify VFS about the
	 * close, it can tell us if the fd was open or not.
	 */
	if (!task->files->fd[fd].vmfile)
		return 0;

	/* Reduce file refcount etc. */
	vm_file_put(task->files->fd[fd].vmfile);

	task->files->fd[fd].vnum = 0;
	task->files->fd[fd].cursor = 0;
	task->files->fd[fd].vmfile = 0;

	return 0;
}

int sys_close(struct tcb *task, int fd)
{
	int ret;

	/* Sync the file and update stats */
	if ((ret = fsync_common(task, fd)) < 0)
		return ret;

	/* Close the file descriptor. */
	return do_close(task, fd);
}

int sys_fsync(struct tcb *task, int fd)
{
	/* Sync the file and update stats */
	return fsync_common(task, fd);
}

/* FIXME: Add error handling to this */
/* Extends a file's size by adding it new pages */
int new_file_pages(struct vm_file *f, unsigned long start, unsigned long end)
{
	unsigned long npages = end - start;
	struct page *page;
	void *paddr;

	/* Allocate the memory for new pages */
	if (!(paddr = alloc_page(npages)))
		return -ENOMEM;

	/* Process each page */
	for (unsigned long i = 0; i < npages; i++) {
		page = phys_to_page(paddr + PAGE_SIZE * i);
		page_init(page);
		page->refcnt++;
		page->owner = &f->vm_obj;
		page->offset = start + i;
		page->virtual = 0;

		/* Add the page to file's vm object */
		BUG_ON(!list_empty(&page->list));
		insert_page_olist(page, &f->vm_obj);
	}

	/* Update vm object */
	f->vm_obj.npages += npages;

	return 0;
}

/* TODO:
 * Re-evaluate. Possibly merge with read_cache_pages.
 */

/* Writes user data in buffer into pages in cache */
int write_cache_pages_orig(struct vm_file *vmfile, struct tcb *task, void *buf,
		      unsigned long pfn_start, unsigned long pfn_end,
		      unsigned long cursor_offset, int count)
{
	struct page *head, *this;
	unsigned long last_pgoff;	/* Last copied page's offset */
	unsigned long copy_offset;	/* Current copy offset on the buffer */
	int copysize, left;

	/* Find the head of consecutive pages */
	list_foreach_struct(head, &vmfile->vm_obj.page_cache, list)
		if (head->offset == pfn_start)
			goto copy;

	/*
	 * Page not found, this is a bug. The writeable page range
	 * must have been readied by read_file_pages()/new_file_pages().
	 */
	BUG();

copy:
	left = count;

	/* Copy the first page and unmap. */
	copysize = (left < PAGE_SIZE) ? left : PAGE_SIZE;
	copy_offset = (unsigned long)buf;
	page_copy(head, task_virt_to_page(task, copy_offset),
		  cursor_offset, copy_offset & PAGE_MASK, copysize);
	head->flags |= VM_DIRTY;
	head->owner->flags |= VM_DIRTY;
	left -= copysize;
	last_pgoff = head->offset;

	/* Map the rest, copy and unmap. */
	list_foreach_struct(this, &head->list, list) {
		if (left == 0 || this->offset == pfn_end)
			break;

		/* Make sure we're advancing on pages consecutively */
		BUG_ON(this->offset != last_pgoff + 1);

		copysize = (left < PAGE_SIZE) ? left : PAGE_SIZE;
		copy_offset = (unsigned long)buf + count - left;

		/* Must be page aligned */
		BUG_ON(!is_page_aligned(copy_offset));

		page_copy(this, task_virt_to_page(task, copy_offset),
			  0, 0, copysize);
		this->flags |= VM_DIRTY;
		left -= copysize;
		last_pgoff = this->offset;
	}
	BUG_ON(left != 0);

	return count - left;
}

/*
 * Writes user data in buffer into pages in cache. If a page is not
 * found, it's a bug. The writeable page range must have been readied
 * by read_file_pages()/new_file_pages().
 */
int write_cache_pages(struct vm_file *vmfile, struct tcb *task, void *buf,
		      unsigned long pfn_start, unsigned long pfn_end,
		      unsigned long cursor_offset, int count)
{
	struct page *head;
	unsigned long last_pgoff;	/* Last copied page's offset */
	unsigned long copy_offset;	/* Current copy offset on the buffer */
	int copysize, left;

	/* Find the head of consecutive pages */
	list_foreach_struct(head, &vmfile->vm_obj.page_cache, list) {
		/* First page */
		if (head->offset == pfn_start) {
			left = count;

			/* Copy the first page and unmap. */
			copysize = (left < PAGE_SIZE) ? left : PAGE_SIZE;
			copy_offset = (unsigned long)buf;
			page_copy(head, task_virt_to_page(task, copy_offset),
				  cursor_offset, copy_offset & PAGE_MASK, copysize);
			head->flags |= VM_DIRTY;
			head->owner->flags |= VM_DIRTY;
			left -= copysize;
			last_pgoff = head->offset;

		/* Rest of the consecutive pages */
		} else if (head->offset > pfn_start && head->offset < pfn_end) {

			/* Make sure we're advancing on pages consecutively */
			BUG_ON(head->offset != last_pgoff + 1);

			copysize = (left < PAGE_SIZE) ? left : PAGE_SIZE;
			copy_offset = (unsigned long)buf + count - left;

			/* Must be page aligned */
			BUG_ON(!is_page_aligned(copy_offset));

			page_copy(head, task_virt_to_page(task, copy_offset),
				  0, 0, copysize);
			head->flags |= VM_DIRTY;
			left -= copysize;
			last_pgoff = head->offset;
		} else if (head->offset == pfn_end || left == 0)
			break;
	}

	BUG_ON(left != 0);

	return count - left;
}

/*
 * Reads a page range from an ordered list of pages into buffer.
 *
 * NOTE:
 * This assumes the page range is consecutively available in the cache
 * and count bytes are available. To ensure this, read_file_pages must
 * be called first and count must be checked. Since it has read-checking
 * assumptions, count must be satisfied.
 */
int read_cache_pages(struct vm_file *vmfile, struct tcb *task, void *buf,
		     unsigned long pfn_start, unsigned long pfn_end,
		     unsigned long cursor_offset, int count)
{
	struct page *head, *this;
	int copysize, left;
	unsigned long last_pgoff;	/* Last copied page's offset */
	unsigned long copy_offset;	/* Current copy offset on the buffer */

	/* Find the head of consecutive pages */
	list_foreach_struct(head, &vmfile->vm_obj.page_cache, list)
		if (head->offset == pfn_start)
			goto copy;

	/* Page not found, nothing read */
	return 0;

copy:
	left = count;

	/* Copy the first page and unmap. */
	copysize = (left < PAGE_SIZE) ? left : PAGE_SIZE;
	copy_offset = (unsigned long)buf;
	page_copy(task_virt_to_page(task, copy_offset), head,
		  PAGE_MASK & copy_offset, cursor_offset, copysize);
	left -= copysize;
	last_pgoff = head->offset;

	/* Map the rest, copy and unmap. */
	list_foreach_struct(this, &head->list, list) {
		if (left == 0 || this->offset == pfn_end)
			break;

		/* Make sure we're advancing on pages consecutively */
		BUG_ON(this->offset != last_pgoff + 1);

		/* Get copying size and start offset */
		copysize = (left < PAGE_SIZE) ? left : PAGE_SIZE;
		copy_offset = (unsigned long)buf + count - left;

		/* MUST be page aligned */
		BUG_ON(!is_page_aligned(copy_offset));

		page_copy(task_virt_to_page(task, copy_offset),
			  this, 0, 0, copysize);
		left -= copysize;
		last_pgoff = this->offset;
	}
	BUG_ON(left != 0);

	return count - left;
}

int sys_read(struct tcb *task, int fd, void *buf, int count)
{
	unsigned long pfn_start, pfn_end;
	unsigned long cursor, byte_offset;
	struct vm_file *vmfile;
	int ret = 0;

	/* Check fd validity */
	if (!task->files->fd[fd].vmfile)
		if ((ret = file_open(task, fd)) < 0)
			return ret;


	/* Check count validity */
	if (count < 0)
		return -EINVAL;
	else if (!count)
		return 0;

	/* Check user buffer validity. */
	if ((ret = pager_validate_user_range(task, buf,
				       (unsigned long)count,
				       VM_READ)) < 0)
		return -EFAULT;

	vmfile = task->files->fd[fd].vmfile;
	cursor = task->files->fd[fd].cursor;

	/* If cursor is beyond file end, simply return 0 */
	if (cursor >= vmfile->length)
		return 0;

	/* Start and end pages expected to be read by user */
	pfn_start = __pfn(cursor);
	pfn_end = __pfn(page_align_up(cursor + count));

	/* But we can read up to maximum file size */
	pfn_end = __pfn(page_align_up(vmfile->length)) < pfn_end ?
		  __pfn(page_align_up(vmfile->length)) : pfn_end;

	/* If trying to read more than end of file, reduce it to max possible */
	if (cursor + count > vmfile->length)
		count = vmfile->length - cursor;

	/* Read the page range into the cache from file */
	if ((ret = read_file_pages(vmfile, pfn_start, pfn_end)) < 0)
		return ret;

	/* The offset of cursor on first page */
	byte_offset = PAGE_MASK & cursor;

	/* Read it into the user buffer from the cache */
	if ((count = read_cache_pages(vmfile, task, buf, pfn_start, pfn_end,
				      byte_offset, count)) < 0)
		return count;

	/* Update cursor on success */
	task->files->fd[fd].cursor += count;

	return count;
}

/* FIXME:
 *
 * Error:
 * We find the page buffer is in, and then copy from the *start* of the page
 * rather than buffer's offset in that page. - I think this is fixed.
 */
int sys_write(struct tcb *task, int fd, void *buf, int count)
{
	unsigned long pfn_wstart, pfn_wend;	/* Write start/end */
	unsigned long pfn_fstart, pfn_fend;	/* File start/end */
	unsigned long pfn_nstart, pfn_nend;	/* New pages start/end */
	unsigned long cursor, byte_offset;
	struct vm_file *vmfile;
	int ret = 0;

	/* Check fd validity */
	if (!task->files->fd[fd].vmfile)
		if ((ret = file_open(task, fd)) < 0)
			return ret;

	/* Check count validity */
	if (count < 0)
		return -EINVAL;
	else if (!count)
		return 0;


	/* Check user buffer validity. */
	if ((ret = pager_validate_user_range(task, buf,
					     (unsigned long)count,
					     VM_WRITE | VM_READ)) < 0)
		return -EINVAL;

	vmfile = task->files->fd[fd].vmfile;
	cursor = task->files->fd[fd].cursor;

	/* See what pages user wants to write */
	pfn_wstart = __pfn(cursor);
	pfn_wend = __pfn(page_align_up(cursor + count));

	/* Get file start and end pages */
	pfn_fstart = 0;
	pfn_fend = __pfn(page_align_up(vmfile->length));

	/*
	 * Find the intersection to determine which pages are
	 * already part of the file, and which ones are new.
	 */
	if (pfn_wstart < pfn_fend) {
		pfn_fstart = pfn_wstart;

		/*
		 * Shorten the end if end page is
		 * less than file size
		 */
		if (pfn_wend < pfn_fend) {
			pfn_fend = pfn_wend;

			/* This also means no new pages in file */
			pfn_nstart = 0;
			pfn_nend = 0;
		} else {

			/* The new pages start from file end,
			 * and end by write end. */
			pfn_nstart = pfn_fend;
			pfn_nend = pfn_wend;
		}

	} else {
		/* No intersection, its all new pages */
		pfn_fstart = 0;
		pfn_fend = 0;
		pfn_nstart = pfn_wstart;
		pfn_nend = pfn_wend;
	}

	/*
	 * Read in the portion that's already part of the file.
	 */
	if ((ret = read_file_pages(vmfile, pfn_fstart, pfn_fend)) < 0)
		return ret;

	/* Create new pages for the part that's new in the file */
	if ((ret = new_file_pages(vmfile, pfn_nstart, pfn_nend)) < 0)
		return ret;

	/*
	 * At this point be it new or existing file pages, all pages
	 * to be written are expected to be in the page cache. Write.
	 */
	byte_offset = PAGE_MASK & cursor;
	if ((ret = write_cache_pages(vmfile, task, buf, pfn_wstart,
				     pfn_wend, byte_offset, count)) < 0)
		return ret;

	/*
	 * Update the file size, and cursor. vfs will be notified
	 * of this change when the file is flushed (e.g. via fflush()
	 * or close())
	 */
	if (task->files->fd[fd].cursor + count > vmfile->length)
		vmfile->length = task->files->fd[fd].cursor + count;

	task->files->fd[fd].cursor += count;

	return count;
}

/* FIXME: Check for invalid cursor values. Check for total, sometimes negative. */
int sys_lseek(struct tcb *task, int fd, off_t offset, int whence)
{
	int retval = 0;
	unsigned long long total, cursor;

	/* Check fd validity */
	if (!task->files->fd[fd].vmfile)
		if ((retval = file_open(task, fd)) < 0)
			return retval;

	/* Offset validity */
	if (offset < 0)
		return -EINVAL;

	switch (whence) {
	case SEEK_SET:
		retval = task->files->fd[fd].cursor = offset;
		break;
	case SEEK_CUR:
		cursor = (unsigned long long)task->files->fd[fd].cursor;
		if (cursor + offset > 0xFFFFFFFF)
			retval = -EINVAL;
		else
			retval = task->files->fd[fd].cursor += offset;
		break;
	case SEEK_END:
		cursor = (unsigned long long)task->files->fd[fd].cursor;
		total = (unsigned long long)task->files->fd[fd].vmfile->length;
		if (cursor + total > 0xFFFFFFFF)
			retval = -EINVAL;
		else {
			retval = task->files->fd[fd].cursor =
				task->files->fd[fd].vmfile->length + offset;
		}
	default:
		retval = -EINVAL;
		break;
	}

	return retval;
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

	BUG(); /* Do this by extended ipc !!! */
	/* Check address is in task's utcb */

	if (fd < 0 || fd > TASK_FILES_MAX || !t->files->fd[fd].vnum)
		return -EBADF;

	vnum = t->files->fd[fd].vnum;

	/* Lookup vnode */
	if (!(v = vfs_lookup_byvnum(vfs_root.pivot->sb, vnum)))
		return -EINVAL;

	d = link_to_struct(v->dentries.next, struct dentry, vref);

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

