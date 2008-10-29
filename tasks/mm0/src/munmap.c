/*
 * munmap() for unmapping a portion of an address space.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <mmap.h>

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
	page->refcnt--;
	BUG_ON(page->refcnt < -1);
	if (page->refcnt == -1) {
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
		if (page->offset >= f_start && page->f_offset <= f_end) {
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

void page_unmap_all_virtuals(struct page *p)
{

}
struct vmo_object *page_to_vm_object(struct page *p)
{
	struct list_head *current_page;

	/* Try to reach head of page list in vm object */
	while (current_page->offset < p->offset) {
		current_page = list_entry(p->list.prev, struct page, list);
		p = current_page;
	}
}

/* TODO:
 * Unmapping virtual address ranges:
 * Implement l4_unmap_range() which would be more efficient than unmapping per-address.
 *
 * Finding virtual mappings of a page:
 * Find a way to obtain all virtual addresses of a physical page.
 * Linux does it by reaching virtual addresses from the page itself,
 * we may rather access virtual addresses from vm_object to vma to page virtual address.
 *
 * OR:
 *
 * We could do this by keeping a list of vma mapping offsets in every object. Then
 * we can find the individual page offsets from those object offsets.
 *
 * Finding unused pages:
 * We need to traverse all vma's that map the vm object to see which pages are unused.
 * We could then swap out file-backed pages, and discard private and anonymous pages.
 */

/* Takes difference of 2 ranges as in the set theory meaning. */
void set_diff_of_ranges(unsigned long *minstart, unsigned long *minend, unsigned long substart, unsigned long subend)
{
	if (*minstart < subend && *minstart > substart)
		*minstart = subend;
	if (start1 >= start2 && start1 < end) {
	}
}

int vmo_unmap_pages(struct tcb *task, struct vm_area *vma, unsigned long start, unsigned long end)
{
	struct vm_obj_link *vmo_link, *n, *link;

	/* Go to each vm object from the vma object chain */
	list_for_each_entry_safe(vmo_link, n, &vma->vm_obj_list, list) {
		/* Go to each link for every object */
		list_for_each_entry(link, &vmo_link->obj->link_list, linkref) {
			/* Go to each vma that owns the link */
			struct vm_area *vma = link->vma;
			unsigned long unused_start, unused_end;

			/* Try every */
			unused_start = max(start, vma->file_offset);
			unused_end = min(end, vma->file_offset +
					 (vma->pfn_end - vma->pfn_start));
		}
	}
}

int vm_obj_unmap_pages(struct vm_object *vmo, unsigned long start, unsigned long end)
{

}

int vma_unmap_objects(struct tcb *task, struct vm_area *vma,
		      unsigned long page_start, unsigned long page_end)
{
	struct vm_obj_link *vmo_link, *n;

	list_for_each_entry_safe(vmo_link, n, &vma->vm_obj_list, list) {
		vm_obj_unmap_pages(vmo_link);
	}
}

/* This shrinks the vma from *one* end only, either start or end */
int vma_shrink(struct vm_area *vma, struct tcb *task,
	       unsigned long pfn_start, unsigned long pfn_end)
{
	unsigned long diff;
	unsigned long obj_unmap_start, obj_unmap_end;

	/* Shrink from the end */
	if (vma->pfn_start < pfn_start) {
		/* Calculate unmap start page */
		obj_unmap_start = vma->f_offset +
				  (pfn_start - vma->pfn_start);

		/* Calculate new vma limits */
		BUG_ON(pfn_start >= vma->pfn_end);
		diff = vma->pfn_end - pfn_start;
		vma->pfn_end = pfn_start;

		/* Calculate unmap end page */
		obj_unmap_end = obj_unmap_start + diff;

	/* Shrink from the beginning */
	} else if (vma->pfn_end > pfn_end) {
		/* Calculate unmap start page */
		obj_unmap_start = vma->f_offset;

		/* Calculate new vma limits */
		BUG_ON(pfn_end <= vma->pfn_start);
		diff = pfn_end - vma->pfn_start;
		vma->f_offset += diff;
		vma->pfn_start = pfn_end;

		/* Calculate unmap end page */
		obj_unmap_end = vma->f_offset;
	} else
		BUG();

	/* Unmap those pages from the task address space */
	task_unmap_region(task, vma, obj_unmap_start, obj_unmap_end);

	/*
	 * The objects underneath may now have resident pages
	 * with no refs, they are released here.
	 */
	vma_release_pages(vma, obj_unmap_start, obj_unmap_end);
}

/*
 * Unmaps the given region from a vma. Depending on the region and vma range,
 * this may result in either shrinking, splitting or destruction of the vma.
 */
int vma_unmap(struct vm_area *vma, struct tcb *task,
	      unsigned long pfn_start, unsigned long pfn_end)
{
	/* Split needed? */
	if (vma->pfn_start < pfn_start && vma->pfn_end > pfn_end) {
		if (!(vma_new = vma_split(vma, task, pfn_start, pfn_end)))
			return -ENOMEM;
		list_add_tail(&vma_new->list, &vma->list);

	/* Shrink needed? */
	} else if (((vma->pfn_start >= pfn_start) && (vma->pfn_end > pfn_end))
	    	   || ((vma->pfn_start < pfn_start) && (vma->pfn_end <= pfn_end)))
		vma_shrink(vma, task, pfn_start, pfn_end);

	/* Destroy needed? */
	else if ((vma->pfn_start >= pfn_start) && (vma->pfn_end <= pfn_end)) {
		/* NOTE: VMA can't be referred after this point. */
		vma_destroy(vma, task);
		vma = 0;
	} else
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

	/* Find a vma that overlaps with this address range */
	while ((vma = find_vma_byrange(munmap_start, munmap_end,
				       &task->vm_area_head.list))) {

		/* Unmap the vma accordingly */
		if ((err = vma_unmap(vma, task, munmap_start,
				     munmap_end)) < 0)
			return err;
	}
	return 0;
}


int sys_munmap(struct tcb *task, void *start, unsigned long length)
{
	/* Must be aligned on a page boundary */
	if ((unsigned long)start & PAGE_MASK)
		return -EINVAL;

	return do_munmap(task, start, __pfn(page_align_up(length)));
}


