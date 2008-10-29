/*
 * munmap() for unmapping a portion of an address space.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <mmap.h>
#include <l4/api/errno.h>
#include <l4lib/arch/syslib.h>

/* TODO: This is to be implemented when fs0 is ready. */
int do_msync(void *addr, unsigned long size, unsigned int flags, struct tcb *task)
{
	return 0;
}

/* This splits a vma, splitter region must be in the *middle* of original vma */
int vma_split(struct vm_area *vma, struct tcb *task,
	      const unsigned long pfn_start, const unsigned long pfn_end)
{
	struct vm_area *new;

	/* Allocate an uninitialised vma first */
	if (!(new = vma_new(0, 0, 0, 0)))
		return -ENOMEM;

	/*
	 * Some sanity checks to show that splitter range does end up
	 * producing two smaller vmas.
	 */
	BUG_ON(vma->pfn_start >= pfn_start || vma->pfn_end <= pfn_end);

	/* Update new and original vmas */
	new->pfn_end = vma->pfn_end;
	new->pfn_start = pfn_end;
	new->file_offset = vma->file_offset + new->pfn_start - vma->pfn_start;
	vma->pfn_end = pfn_start;
	new->flags = vma->flags;

	/* Add new one next to original vma */
	list_add_tail(&new->list, &vma->list);

	return 0;
}

/* This shrinks the vma from *one* end only, either start or end */
int vma_shrink(struct vm_area *vma, struct tcb *task,
	       const unsigned long pfn_start, const unsigned long pfn_end)
{
	unsigned long diff;

	/* Shrink from the end */
	if (vma->pfn_start < pfn_start) {
		BUG_ON(pfn_start >= vma->pfn_end);
		vma->pfn_end = pfn_start;

	/* Shrink from the beginning */
	} else if (vma->pfn_end > pfn_end) {
		BUG_ON(pfn_end <= vma->pfn_start);
		diff = pfn_end - vma->pfn_start;
		vma->file_offset += diff;
		vma->pfn_start = pfn_end;
	} else BUG();

	return 0;
}

/*
 * Unmaps the given region from a vma. Depending on the region and vma range,
 * this may result in either shrinking, splitting or destruction of the vma.
 */
int vma_unmap(struct vm_area *vma, struct tcb *task,
	      const unsigned long pfn_start, const unsigned long pfn_end)
{
	/* Split needed? */
	if (vma->pfn_start < pfn_start && vma->pfn_end > pfn_end)
		return vma_split(vma, task, pfn_start, pfn_end);
	/* Shrink needed? */
	else if (((vma->pfn_start >= pfn_start) && (vma->pfn_end > pfn_end))
	    	   || ((vma->pfn_start < pfn_start) && (vma->pfn_end <= pfn_end)))
		return vma_shrink(vma, task, pfn_start, pfn_end);
	/* Destroy needed? */
	else if ((vma->pfn_start >= pfn_start) && (vma->pfn_end <= pfn_end))
		return vma_drop_merge_delete_all(vma);
	else
		BUG();

	return 0;
}

/*
 * Unmaps the given virtual address range from the task, the region
 * may span into zero or more vmas, and may involve shrinking, splitting
 * and destruction of multiple vmas.
 */
int do_munmap(struct tcb *task, void *vaddr, unsigned long npages)
{
	const unsigned long munmap_start = __pfn(vaddr);
	const unsigned long munmap_end = munmap_start + npages;
	struct vm_area *vma;
	int err;

	/* Find a vma that overlaps with this address range */
	while ((vma = find_vma_byrange(munmap_start, munmap_end,
				       &task->vm_area_head->list))) {

		/* Unmap the vma accordingly */
		if ((err = vma_unmap(vma, task, munmap_start,
				     munmap_end)) < 0)
			return err;
	}

	/* Unmap those pages from the task address space */
	l4_unmap(vaddr, npages, task->tid);

	return 0;
}


int sys_munmap(struct tcb *task, void *start, unsigned long length)
{
	/* Must be aligned on a page boundary */
	if ((unsigned long)start & PAGE_MASK)
		return -EINVAL;

	return do_munmap(task, start, __pfn(page_align_up(length)));
}


