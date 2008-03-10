/*
 * Copyright (C) 2008 Bahadir Balban
 */
#include <init.h>
#include <vm_area.h>
#include <kmalloc/kmalloc.h>
#include <l4/macros.h>
#include <l4/api/errno.h>
#include <l4lib/types.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/kip.h>
#include <posix/sys/types.h>
#include <string.h>


int vfs_read(unsigned long vnum, unsigned long f_offset, unsigned long npages,
	     void *pagebuf)
{
	int err;

	write_mr(L4SYS_ARG0, vnum);
	write_mr(L4SYS_ARG1, f_offset);
	write_mr(L4SYS_ARG2, npages);
	write_mr(L4SYS_ARG3, (u32)pagebuf);

	if ((err = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_READ)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	/* Check if syscall was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: Pager from VFS read error: %d.\n", __FUNCTION__, err);
		return err;
	}

	return err;
}

int vfs_write(unsigned long vnum, unsigned long f_offset, unsigned long npages,
	      void *pagebuf)
{
	int err;

	write_mr(L4SYS_ARG0, vnum);
	write_mr(L4SYS_ARG1, f_offset);
	write_mr(L4SYS_ARG2, npages);
	write_mr(L4SYS_ARG3, (u32)pagebuf);

	if ((err = l4_sendrecv(VFS_TID, VFS_TID, L4_IPC_TAG_WRITE)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	/* Check if syscall was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: Pager to VFS write error: %d.\n", __FUNCTION__, err);
		return err;
	}

	return err;
}

/*
 * When a new file is opened by the vfs this receives the information
 * about that file so that it can serve that file's content (via
 * read/write/mmap) later to that task.
 */
int vfs_receive_sys_open(l4id_t sender, l4id_t opener, int fd,
			 unsigned long vnum, unsigned long length)
{
	struct vm_file *vmfile;
	struct tcb *t;

	/* Check argument validity */
	if (sender != VFS_TID)
		return -EPERM;

	if (!(t = find_task(opener)))
		return -EINVAL;

	if (fd < 0 || fd > TASK_OFILES_MAX)
		return -EINVAL;

	/* Assign vnum to given fd on the task */
	t->fd[fd].vnum = vnum;
	t->fd[fd].cursor = 0;

	/* Check if that vm_file is already in the list */
	list_for_each_entry(vmfile, &vm_file_list, list) {
		if (vmfile->vnum == vnum) {
			/* Add a reference to it from the task */
			t->fd[fd].vmfile = vmfile;
			vmfile->refcnt++;
			return 0;
		}
	}

	/* Otherwise allocate a new one for this vnode */
	if (IS_ERR(vmfile = vmfile_alloc_init()))
		return (int)vmfile;

	/* Initialise and add it to global list */
	vmfile->vnum = vnum;
	vmfile->length = length;
	vmfile->pager = &default_file_pager;
	list_add(&vmfile->list, &vm_file_list);

	return 0;
}


/*
 * Inserts the page to vmfile's list in order of page frame offset.
 * We use an ordered list instead of a radix or btree for now.
 */
int insert_page_olist(struct page *this, struct vm_object *vmo)
{
	struct page *before, *after;

	/* Add if list is empty */
	if (list_empty(&vmo->page_cache)) {
		list_add_tail(&this->list, &vmo->page_cache);
		return 0;
	}
	/* Else find the right interval */
	list_for_each_entry(before, &vmo->page_cache, list) {
		after = list_entry(before->list.next, struct page, list);

		/* If there's only one in list */
		if (before->list.next == &vmo->page_cache) {
			/* Add to end if greater */
			if (this->offset > before->offset)
				list_add_tail(&this->list, &before->list);
			/* Add to beginning if smaller */
			else if (this->offset < before->offset)
				list_add(&this->list, &before->list);
			else
				BUG();
			return 0;
		}

		/* If this page is in-between two other, insert it there */
		if (before->offset < this->offset &&
		    after->offset > this->offset) {
			list_add_tail(&this->list, &before->list);
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
		/* The page is not resident in page cache. */
		if (!(page = find_page(vmfile, f_offset))) {
			/* Allocate a new page */
			void *paddr = alloc_page(1);
			void *vaddr = phys_to_virt(paddr);
			page = phys_to_page(paddr);

			/*
			 * Map new page at a self virtual address.
			 * NOTE: this is not unmapped here but in
			 * read_cache_pages where mm0's work with the
			 * page is done.
			 */
			l4_map(paddr, vaddr, 1, MAP_USR_RW_FLAGS, self_tid());

			/* Read-in the page using the file's pager */
			vmfile->pager->ops.read_page(vmfile, f_offset, vaddr);

			spin_lock(&page->lock);
			page->count++;
			page->owner = vmfile;
			page->f_offset = f_offset;
			page->virtual = 0;

			/* Add the page to owner's list of in-memory pages */
			BUG_ON(!list_empty(&page->list));
			insert_page_olist(page, vmfile);
			spin_unlock(&page->lock);
		}
	}

	return 0;
}

/* Reads a page range from an ordered list of pages into buffer */
int read_cache_pages(struct vm_file *vmfile, void *buf, unsigned long pfn_start,
		     unsigned long pfn_end, unsigned long offset,
		     int count)
{
	struct page *head, *next;
	int copysize, left;
	void *page_virtual;

	list_for_each_entry(head, &vmfile->page_cache_list, list)
		if (head->f_offset == pfn_start)
			goto copy;
	BUG();

copy:
	left = count;

	/*
	 * This function assumes the pages are already in-memory and
	 * they are mapped into the current address space.
	 */
	page_virtual = phys_to_virt((void *)page_to_phys(head));

	/* Copy the first page and unmap it from current task. */
	copysize = (left <= PAGE_SIZE) ? left : PAGE_SIZE;
	memcpy(buf, page_virtual + offset,
	       copysize);
	left -= copysize;
	l4_unmap(page_virtual, 1, self_tid());

	/* Copy the rest and unmap. */
	list_for_each_entry(next, &head->list, list) {
		if (left == 0 || next->f_offset == pfn_end)
			break;
		copysize = (left <= PAGE_SIZE) ? left : PAGE_SIZE;
		page_virtual = phys_to_virt((void *)page_to_phys(next));
		memcpy(buf + count - left, page_virtual, copysize);
		l4_unmap(page_virtual, 1, self_tid());
		left -= copysize;
	}
	BUG_ON(left != 0);

	return 0;
}


int sys_read(l4id_t sender, int fd, void *buf, int count)
{
	unsigned long foff_pfn_start, foff_pfn_end;
	unsigned long cursor, first_pgoff;
	struct vm_file *vmfile;
	struct tcb *t;
	int err;

	BUG_ON(!(t = find_task(sender)));

	/* TODO: Check user buffer and count validity */
	if (fd < 0 || fd > TASK_OFILES_MAX)
		return -EINVAL;

	vmfile = t->fd[fd].vmfile;
	cursor = t->fd[fd].cursor;

	foff_pfn_start = __pfn(cursor);
	foff_pfn_end = __pfn(page_align_up(cursor + count));

	/* Read the page range into the cache from file */
	if ((err =  read_file_pages(vmfile, foff_pfn_start, foff_pfn_end) < 0))
		return err;

	/* The offset of cursor on first page */
	first_pgoff = PAGE_MASK & cursor;

	/* Read it into the user buffer from the cache */
	if ((err = read_cache_pages(vmfile, buf, foff_pfn_start, foff_pfn_end,
				    first_pgoff, count)) < 0)
		return err;

	/* Update cursor on success */
	t->fd[fd].cursor += count;

	return 0;
}

int sys_write(l4id_t sender, int fd, void *buf, int count)
{
	return 0;
}

/* FIXME: Check for invalid cursor values */
int sys_lseek(l4id_t sender, int fd, off_t offset, int whence)
{
	struct tcb *t;

	BUG_ON(!(t = find_task(sender)));

	if (offset < 0)
		return -EINVAL;

	switch (whence) {
		case SEEK_SET:
			t->fd[fd].cursor = offset;
			break;
		case SEEK_CUR:
			t->fd[fd].cursor += offset;
			break;
		case SEEK_END:
			t->fd[fd].cursor = t->fd[fd].vmfile->length + offset;
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

