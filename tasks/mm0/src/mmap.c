/*
 * mmap/munmap and friends.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <vm_area.h>
#include <kmalloc/kmalloc.h>
#include INC_API(errno.h)
#include <posix/sys/types.h>
#include <task.h>
#include <mmap.h>
#include <memory.h>
#include <l4lib/arch/syscalls.h>


/* TODO: This is to be implemented when fs0 is ready. */
int do_msync(void *addr, unsigned long size, unsigned int flags, struct tcb *task)
{
	// unsigned long npages = __pfn(size);
	struct vm_area *vma = find_vma((unsigned long)addr,
				       &task->vm_area_list);
	if (!vma)
		return -EINVAL;

	/* Must check if this is a shadow copy or not */
	if (vma->flags & VMA_COW) {
		;	/* ... Fill this in. ... */
	}

	/* TODO:
	 * Flush the vma's pages back to their file. Perhaps add a dirty bit
	 * to the vma so that this can be completely avoided for clean vmas?
	 * For anon pages this is the swap file. For real file-backed pages
	 * its the real file. However, this can't be fully implemented yet since
	 * we don't have FS0 yet.
	 */
	return 0;
}

/*
 * This releases a physical page struct from its owner and
 * frees the page back to the page allocator.
 */
int page_release(struct page *page)
{
	spin_lock(&page->lock);
	page->count--;
	BUG_ON(page->count < -1);
	if (page->count == -1) {
		/* Unlink the page from its owner's list */
		list_del_init(&page->list);

		/* Zero out the fields */
		page->owner = 0;
		page->flags = 0;
		page->f_offset = 0;
		page->virtual = 0;

		/*
		 * No refs to page left, and since every physical memory page
		 * comes from the page allocator, we return it back.
		 */
		free_page((void *)page_to_phys(page));
	}
	spin_unlock(&page->lock);
	return 0;
}

/*
 * Freeing and unmapping of vma pages:
 *
 * For a vma that is about to be split, shrunk or destroyed, this function
 * finds out about the physical pages in memory that represent the vma,
 * reduces their refcount, and if they're unused, frees them back to the
 * physical page allocator, and finally unmaps those corresponding virtual
 * addresses from the unmapper task's address space. This sequence is
 * somewhat a rewinding of the actions that the page fault handler takes
 * when the vma was faulted by the process.
 */
int vma_release_pages(struct vm_area *vma, struct tcb *task,
		      unsigned long pfn_start, unsigned long pfn_end)
{
	unsigned long f_start, f_end;
	struct page *page, *n;

	/* Assume vma->pfn_start is lower than or equal to pfn_start */
	BUG_ON(vma->pfn_start > pfn_start);

	/* Assume vma->pfn_end is higher or equal to pfn_end */
	BUG_ON(vma->pfn_end < pfn_end);

	/* Find the file offsets of the range to be freed. */
	f_start = vma->f_offset + pfn_start - vma->pfn_start;
	f_end = vma->f_offset + vma->pfn_end - pfn_end;

	list_for_each_entry_safe(page, n, &vma->owner->page_cache_list, list) {
		if (page->f_offset >= f_start && page->f_offset <= f_end) {
			l4_unmap((void *)virtual(page), 1, task->tid);
			page_release(page);
		}
	}
	return 0;
}
int vma_unmap(struct vm_area **orig, struct vm_area **new,
	      unsigned long, unsigned long, struct tcb *);
/*
 * This is called by every vma modifier function in vma_unmap(). This in turn
 * calls vma_unmap recursively to modify the shadow vmas, the same way the
 * actual vmas get modified. Only COW vmas would need to do this recursion
 * and the max level of recursion is one, since only one level of shadows exist.
 */
int vma_unmap_shadows(struct vm_area *vma, struct tcb *task, unsigned long pfn_start,
		      unsigned long pfn_end)
{
	struct vm_area *shadow, *n;

	/* Now do all shadows */
	list_for_each_entry_safe(shadow, n, &vma->shadow_list,
				 shadow_list) {
		BUG_ON(!(vma->flags & VMA_COW));
		if (shadow->pfn_start >= pfn_start &&
		    shadow->pfn_end <= pfn_end) {
			struct vm_area *split_shadow;
			/* This may result in shrink/destroy/split of the shadow */
			vma_unmap(&shadow, &split_shadow, pfn_start, pfn_end, task);
			if (shadow && split_shadow)
				list_add_tail(&split_shadow->list,
					      &shadow->list);
			/* FIXME: Is this all to be done here??? Find what to do here. */
			BUG();
		}
	}
	return 0;
}

struct vm_area *vma_new(unsigned long pfn_start, unsigned long npages,
			unsigned int flags,  unsigned long f_offset,
			struct vm_file *owner)
{
	struct vm_area *vma;

	/* Initialise new area */
	if (!(vma = kzalloc(sizeof(struct vm_area))))
		return 0;

	vma->pfn_start = pfn_start;
	vma->pfn_end = pfn_start + npages;
	vma->flags = flags;
	vma->f_offset = f_offset;
	vma->owner = owner;
	INIT_LIST_HEAD(&vma->list);
	INIT_LIST_HEAD(&vma->shadow_list);

	return vma;
}

/* TODO: vma_destroy/shrink/split should also handle swap file modification */

/* Frees and unlinks a vma from its list. TODO: Add list locking */
int vma_destroy(struct vm_area *vma, struct tcb *task)
{
	struct vm_area *shadow, *n;

	/* Release the vma pages */
	vma_release_pages(vma, task, vma->pfn_start, vma->pfn_end);

	/* Free all shadows, if any. */
	list_for_each_entry_safe(shadow, n, &vma->shadow_list, list) {
		/* Release all shadow pages */
		vma_release_pages(shadow, task, shadow->pfn_start, shadow->pfn_end);
		list_del(&shadow->list);
		kfree(shadow);
	}

	/* Unlink and free the vma itself */
	list_del(&vma->list);
	if (kfree(vma) < 0)
		BUG();

	return 0;
}

/* This splits a vma, splitter region must be in the *middle* of original vma */
struct vm_area *vma_split(struct vm_area *vma, struct tcb *task,
			  unsigned long pfn_start, unsigned long pfn_end)
{
	struct vm_area *new, *shadow, *n;

	/* Allocate an uninitialised vma first */
	if (!(new = vma_new(0, 0, 0, 0, 0)))
		return 0;

	/*
	 * Some sanity checks to show that splitter range does end up
	 * producing two smaller vmas.
	 */
	BUG_ON(vma->pfn_start >= pfn_start || vma->pfn_end <= pfn_end);

	/* Release the pages before modifying the original vma */
	vma_release_pages(vma, task, pfn_start, pfn_end);

	new->pfn_end = vma->pfn_end;
	new->pfn_start = pfn_end;
	new->f_offset = vma->f_offset + new->pfn_start - vma->pfn_start;
	vma->pfn_end = pfn_start;

	new->flags = vma->flags;
	new->owner = vma->owner;

	/* Modify the shadows accordingly first. They may
	 * split/shrink or get completely destroyed or stay still. */
	vma_unmap_shadows(vma, task, pfn_start, pfn_end);

	/*
	 * Now split the modified shadows list into two vmas:
	 * If the file was COW and its vma had split, vma_new would have
	 * a valid value and as such the shadows must be separated into
	 * the two new vmas according to which one they belong to.
	 */
	list_for_each_entry_safe(shadow, n, &vma->shadow_list,
				 shadow_list) {
		BUG_ON(!(vma->flags & VMA_COW));
		BUG_ON(!(new->flags & VMA_COW));
		if (shadow->pfn_start >= new->pfn_start &&
		    shadow->pfn_end <= new->pfn_end) {
			list_del_init(&shadow->list);
			list_add(&shadow->list, &new->shadow_list);
		} else
			BUG_ON(!(shadow->pfn_start >= vma->pfn_start &&
			       shadow->pfn_end <= vma->pfn_end));
	}

	return new;
}

/* This shrinks the vma from *one* end only, either start or end */
int vma_shrink(struct vm_area *vma, struct tcb *task, unsigned long pfn_start,
	       unsigned long pfn_end)
{
	unsigned long diff;

	BUG_ON(pfn_start >= pfn_end);

	/* FIXME: Shadows are currently buggy - TBD */
	if (!list_empty(&vma->shadow_list)) {
		BUG();
		vma_swapfile_realloc(vma, pfn_start, pfn_end);
		return 0;
	}

	/* Release the pages before modifying the original vma */
	vma_release_pages(vma, task, pfn_start, pfn_end);

	/* Shrink from the beginning */
	if (pfn_start > vma->pfn_start) {
		diff = pfn_start - vma->pfn_start;
		vma->f_offset += diff;
		vma->pfn_start = pfn_start;

	/* Shrink from the end */
	} else if (pfn_end < vma->pfn_end) {
		diff = vma->pfn_end - pfn_end;
		vma->pfn_end = pfn_end;
	} else
		BUG();

	return vma_unmap_shadows(vma, task, pfn_start, pfn_end);
}

/*
 * Unmaps the given region from a vma. Depending on the region and vma range,
 * this may result in either shrinking, splitting or destruction of the vma.
 */
int vma_unmap(struct vm_area **actual, struct vm_area **split,
	      unsigned long pfn_start, unsigned long pfn_end, struct tcb *task)
{
	struct vm_area *vma = *actual;
	struct vm_area *vma_new = 0;

	/* Split needed? */
	if (vma->pfn_start < pfn_start && vma->pfn_end > pfn_end) {
		if (!(vma_new = vma_split(vma, task, pfn_start, pfn_end)))
			return -ENOMEM;
		list_add_tail(&vma_new->list, &vma->list);

	/* Shrink needed? */
	} else if (((vma->pfn_start == pfn_start) && (vma->pfn_end > pfn_end))
	    	   || ((vma->pfn_start < pfn_start) && (vma->pfn_end == pfn_end)))
		vma_shrink(vma, task, pfn_start, pfn_end);

	/* Destroy needed? */
	else if ((vma->pfn_start >= pfn_start) && (vma->pfn_end <= pfn_end)) {
		/* NOTE: VMA can't be referred after this point. */
		vma_destroy(vma, task);
		vma = 0;
	} else
		BUG();

	/* Update actual pointers */
	*actual = vma;
	*split = vma_new;

	return 0;
}

/* Unmaps given address range from its vma. Releases those pages in that vma. */
int do_munmap(void *vaddr, unsigned long size, struct tcb *task)
{
	unsigned long npages = __pfn(size);
	unsigned long pfn_start = __pfn(vaddr);
	unsigned long pfn_end = pfn_start + npages;
	struct vm_area *vma, *vma_new = 0;
	int err;

	/* Check if any such vma exists */
	if (!(vma = find_vma((unsigned long)vaddr, &task->vm_area_list)))
		return -EINVAL;

	/*
	 * If end of the range is outside of the vma that has the start
	 * address, we ignore the rest and assume end is the end of that vma.
	 * TODO: Find out how posix handles this.
	 */
	if (pfn_end > vma->pfn_end) {
		printf("%s: %s: Warning, unmap end 0x%x beyond vma range. "
		       "Ignoring.\n", __TASKNAME__, __FUNCTION__,
		       __pfn_to_addr(pfn_end));
		pfn_end = vma->pfn_end;
	}
	if ((err = vma_unmap(&vma, &vma_new, pfn_start, pfn_end, task)) < 0)
		return err;
#if 0
mod_phys_pages:

	/* The stage where the actual pages are unmapped from the page tables */
pgtable_unmap:

	/* TODO:
	 * - Find out if the vma is cow, and contains shadow vmas.
	 * - Remove and free shadow vmas or the real vma, or shrink them if applicable.
	 * - Free the swap file segment for the vma if vma is private (cow).
	 * - Reduce refcount for the in-memory pages.
	 * - If refcount is zero (they could be shared!), either add pages to some page
	 *   cache, or simpler the better, free the actual pages back to the page allocator.
	 * - l4_unmap() the corresponding virtual region from the page tables.
	 *
	 *   -- These are all done --
	 */
#endif
	return 0;
}

int sys_munmap(l4id_t sender, void *vaddr, unsigned long size)
{
	struct tcb *task;

	BUG_ON(!(task = find_task(sender)));

	return do_munmap(vaddr, size, task);
}

static struct vm_area *
is_vma_mergeable(unsigned long pfn_start, unsigned long pfn_end,
		 unsigned int flags, struct vm_area *vma)
{
	/* TODO:
	 * The swap implementation is too simple for now. The vmas on swap
	 * are stored non-sequentially, and adjacent vmas don't imply adjacent
	 * file position on swap. So at the moment merging swappable vmas
	 * doesn't make sense. But this is going to change in the future.
	 */
	if (vma->flags & VMA_COW) {
		BUG();
		/* FIXME: XXX: Think about this! */
	}

	/* Check for vma adjacency */
	if ((vma->pfn_start == pfn_end) && (vma->flags == flags))
		return vma;
	if ((vma->pfn_end == pfn_start) && (vma->flags == flags))
		return vma;

	return 0;
}

/*
 * Finds an unmapped virtual memory area for the given parameters. If it
 * overlaps with an existing vma, it returns -1, if it is adjacent to an
 * existing vma and the flags match, it returns the adjacent vma. Otherwise it
 * returns 0.
 */
int find_unmapped_area(struct vm_area **existing,  unsigned long pfn_start,
		       unsigned long npages, unsigned int flags,
		       struct list_head *vm_area_head)
{
	unsigned long pfn_end = pfn_start + npages;
	struct vm_area *vma;
	*existing = 0;

	list_for_each_entry(vma, vm_area_head, list) {
		/* Check overlap */
		if ((vma->pfn_start <= pfn_start) &&
		    (pfn_start < vma->pfn_end)) {
			printf("%s: VMAs overlap.\n", __FUNCTION__);
			return -1;	/* Overlap */
		} if ((vma->pfn_start < pfn_end) &&
		    (pfn_end < vma->pfn_end)) {
			printf("%s: VMAs overlap.\n", __FUNCTION__);
			return -1;	/* Overlap */
		}
		if (is_vma_mergeable(pfn_start, pfn_end, flags, vma)) {
			*existing = vma;
			return 0;
		}
	}
	return 0;
}

/*
 * Maps the given file with given flags at the given page offset to the given
 * task's address space at the specified virtual memory address and length.
 *
 * The actual paging in/out of the file from/into memory pages is handled by
 * the file's pager upon page faults.
 */
int do_mmap(struct vm_file *mapfile, unsigned long f_offset, struct tcb *t,
	    unsigned long map_address, unsigned int flags, unsigned int pages)
{
	struct vm_area *vma;
	struct vm_object *vmobj;
	unsigned long pfn_start = __pfn(map_address);

	if (!mapfile) {
	       if (flags & VMA_ANON) {
			vmobj = get_devzero();
			f_offset = 0;
	       } else
			BUG();
	} else {
		if (pages > (__pfn(page_align_up(mapfile->length))
			     - f_offset)) {
			printf("%s: Trying to map %d pages from page %d, "
			       "but file length is %d\n", __FUNCTION__, pages,
			       f_offset, __pfn(page_align_up(mapfile->length)));
			return -EINVAL;
		}
		/* Set up a vm object for given file */
		vmobj = vm_obj_alloc_init();
		vmobj->priv.file = mapfile;
	}

	printf("%s: Mapping 0x%x - 0x%x\n", __FUNCTION__, map_address,
	       map_address + pages * PAGE_SIZE);

	/* See if it overlaps or is mergeable to an existing vma. */
	if (find_unmapped_area(&vma, pfn_start, pages, flags,
			       &t->vm_area_list) < 0)
		return -EINVAL;	/* Indicates overlap. */

	/* Mergeable vma returned? */
	if (vma) {
		if (vma->pfn_end == pfn_start)
			vma->pfn_end = pfn_start + pages;
		else {
			vma->f_offset -= vma->pfn_start - pfn_start;

			/* Check if adjusted yields the original */
			BUG_ON(vma->f_offset != f_offset);
			vma->pfn_start = pfn_start;
		}
	} else { /* Initialise new area */
		if (!(vma = vma_new(pfn_start, pages, flags, f_offset,
				    mapfile)))
			return -ENOMEM;
		list_add(&vma->list, &t->vm_area_list);
	}
	return 0;
}

/* mmap system call implementation */
int sys_mmap(l4id_t sender, void *start, size_t length, int prot,
	     int flags, int fd, unsigned long pfn)
{
	unsigned long npages = __pfn(page_align_up(length));
	struct tcb * task;
	int err;

	BUG_ON(!(task = find_task(sender)));

	if (fd < 0 || fd > TASK_OFILES_MAX)
		return -EINVAL;

	if ((unsigned long)start < USER_AREA_START || (unsigned long)start >= USER_AREA_END)
		return -EINVAL;

	/* TODO:
	 * Check that @start does not already have a mapping.
	 * Check that pfn + npages range is within the file range.
	 * Check that posix flags passed match those defined in vm_area.h
	 */
	if ((err =  do_mmap(task->fd[fd].vmfile, __pfn_to_addr(pfn), task,
			    (unsigned long)start, flags, npages)) < 0)
		return err;

	return 0;
}

/* Sets the end of data segment for sender */
int sys_brk(l4id_t sender, void *ds_end)
{
	return 0;
}

