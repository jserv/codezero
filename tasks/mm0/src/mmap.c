/*
 * mmap/munmap and friends.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <vm_area.h>
#include <kmalloc/kmalloc.h>
#include INC_API(errno.h)
#include <posix/sys/types.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <task.h>
#include <mmap.h>
#include <file.h>
#include <memory.h>

#if 0
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
int do_munmap(void *vaddr, unsigned long npages, struct tcb *task)
{
	unsigned long pfn_start = __pfn(vaddr);
	unsigned long pfn_end = pfn_start + npages;
	struct vm_area *vma, *vma_new = 0;
	int err;

	/* Check if any such vma exists */
	if (!(vma = find_vma((unsigned long)vaddr, &task->vm_area_head.list)))
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
#endif


int do_munmap(void *vaddr, unsigned long npages, struct tcb *task)
{
	return 0;
}

int sys_munmap(l4id_t sender, void *vaddr, unsigned long size)
{
	struct tcb *task;

	BUG_ON(!(task = find_task(sender)));

	return do_munmap(vaddr, __pfn(page_align_up(size)), task);
}

struct vm_area *vma_new(unsigned long pfn_start, unsigned long npages,
			unsigned int flags, unsigned long file_offset)
{
	struct vm_area *vma;

	/* Allocate new area */
	if (!(vma = kzalloc(sizeof(struct vm_area))))
		return 0;

	INIT_LIST_HEAD(&vma->list);
	INIT_LIST_HEAD(&vma->vm_obj_list);

	vma->pfn_start = pfn_start;
	vma->pfn_end = pfn_start + npages;
	vma->flags = flags;
	vma->file_offset = file_offset;

	return vma;
}

int vma_intersect(unsigned long pfn_start, unsigned long pfn_end,
		      struct vm_area *vma)
{
	if ((pfn_start <= vma->pfn_start) && (pfn_end > vma->pfn_start)) {
		printf("%s: VMAs overlap.\n", __FUNCTION__);
		return 1;
	}
	if ((pfn_end >= vma->pfn_end) && (pfn_start < vma->pfn_end)) {
		printf("%s: VMAs overlap.\n", __FUNCTION__);
		return 1;
	}
	return 0;
}

/*
 * FIXME: PASS THIS A VM_SHARED FLAG SO THAT IT CAN SEARCH FOR AN EMPTY
 * SEGMENT FOR SHM, instead of shmat() searching for one.
 *
 * Search an empty space in the task's mmapable address region.
 */
unsigned long find_unmapped_area(unsigned long npages, struct tcb *task)
{
	unsigned long pfn_start = __pfn(task->map_start);
	unsigned long pfn_end = pfn_start + npages;
	struct vm_area *vma;

	if (npages > __pfn(task->map_end - task->map_start))
		return 0;

	/* If no vmas, first map slot is available. */
	if (list_empty(&task->vm_area_head->list))
		return task->start;

	/* First vma to check our range against */
	vma = list_entry(task->vm_area_head->list.next, struct vm_area, list);

	/* Start searching from task's end of data to start of stack */
	while (pfn_end <= __pfn(task->end)) {

		/* If intersection, skip the vma and fast-forward to next */
		if (vma_intersect(pfn_start, pfn_end, vma)) {

			/* Update interval to next available space */
			pfn_start = vma->pfn_end;
			pfn_end = pfn_start + npages;

			/*
			 * Decision point, no more vmas left to check.
			 * Are we out of task map area?
			 */
			if (vma->list.next == &task->vm_area_head->list) {
				if (pfn_end > __pfn(task->end))
					break; /* Yes, fail */
				else	/* No, success */
					return __pfn_to_addr(pfn_start);
			}

			/* Otherwise get next vma entry */
			vma = list_entry(vma->list.next,
					 struct vm_area, list);
			continue;
		}
		BUG_ON(pfn_start + npages > __pfn(task->end));
		return __pfn_to_addr(pfn_start);
	}

	return 0;
}

/* Validate an address that is a possible candidate for an mmap() region */
int mmap_address_validate(struct tcb *task, unsigned long map_address,
			  unsigned int vm_flags)
{
	if (map_address == 0)
		return 0;

	/* Private mappings can only go in task address space */
	if (vm_flags & VMA_PRIVATE) {
		if (map_address >= task->start ||
	    	    map_address < task->end) {
			return 1;
		} else
			return 0;
	/*
	 * Shared mappings can go in task, utcb, and shared
	 * memory address space,
	 */
	} else if (vm_flags & VMA_SHARED) {
		if ((map_address >= UTCB_AREA_START &&
		     map_address < UTCB_AREA_END) ||
		    (map_address >= task->start &&
	    	     map_address < task->end) ||
		    (map_address >= SHM_AREA_START &&
	    	     map_address < SHM_AREA_END))
			return 1;
		else
			return 0;
	} else
		BUG();
}

/*
 * Maps the given file with given flags at the given page offset to the given
 * task's address space at the specified virtual memory address and length.
 *
 * The actual paging in/out of the file from/into memory pages is handled by
 * the file's pager upon page faults.
 */
void *do_mmap(struct vm_file *mapfile, unsigned long file_offset,
	      struct tcb *task, unsigned long map_address, unsigned int flags,
	      unsigned int npages)
{
	unsigned long map_pfn = __pfn(map_address);
	struct vm_area *new, *mapped;
	struct vm_obj_link *vmo_link, *vmo_link2;
	unsigned long file_npages;

	/* Set up devzero if none given */
	if (!mapfile) {
	       if (flags & VMA_ANONYMOUS) {
			BUG_ON(!(mapfile = get_devzero()));
			file_offset = 0;
	       } else
		       return PTR_ERR(-EINVAL);
	}

	/* Get total file pages, check if mapping is within file size */
	file_npages = __pfn(page_align_up(mapfile->length));
	if (npages > file_npages - file_offset) {
		printf("%s: Trying to map %d pages from page %d, "
		       "but file length is %d\n", __FUNCTION__,
		       npages, file_offset, file_npages);
		return PTR_ERR(-EINVAL);
	}

	/* Check invalid page size */
	if (npages == 0) {
		printf("Trying to map %d pages.\n", npages);
		return PTR_ERR(-EINVAL);
	}
	if (npages > __pfn(task->stack_start - task->data_end)) {
		printf("Trying to map too many pages: %d\n", npages);
		return PTR_ERR(-ENOMEM);
	}

	/* Check invalid map address */
	if (!mmap_address_validate(task, map_address, flags)) {
		/* Get new map address for region of this size */
		if(!(map_address = find_unmapped_area(npages, task)))
			return PTR_ERR(-ENOMEM);
	} else {
		/*
		 * FIXME: Currently we don't allow overlapping vmas.
		 * To be fixed soon. We need to handle intersection,
		 * splitting, shrink/grow etc.
		 */
		list_for_each_entry(mapped, &task->vm_area_head->list, list)
			BUG_ON(vma_intersect(map_pfn, map_pfn + npages,
					     mapped));
	}

	/* For valid regions that aren't allocated by us, create the vma. */
	if (!(new = vma_new(__pfn(map_address), npages, flags, file_offset)))
		return PTR_ERR(-ENOMEM);

	/* Attach the file as the first vm object of this vma */
	if (!(vmo_link = vm_objlink_create())) {
		kfree(new);
		return PTR_ERR(-ENOMEM);
	}

	/* Attach link to object */
	vm_link_object(vmo_link, &mapfile->vm_obj);

	/* Add link to vma list */
	list_add_tail(&vmo_link->list, &new->vm_obj_list);

	/*
	 * If the file is a shm file, also map devzero behind it. i.e.
	 * vma -> vm_link -> vm_link
	 * 	     |          |
	 * 	     v          v
	 * 	  shm_file	devzero
	 *
	 * So that faults go through shm file and then devzero, as in
	 * the shadow object copy_on_write setup in fault.c
	 */
	if (mapfile->type == VM_FILE_SHM) {
		struct vm_file *dzero = get_devzero();

		/* Attach the file as the first vm object of this vma */
		if (!(vmo_link2 = vm_objlink_create())) {
			kfree(new);
			kfree(vmo_link);
			return PTR_ERR(-ENOMEM);
		}
		vm_link_object(vmo_link2, &dzero->vm_obj);
		list_add_tail(&vmo_link2->list, &new->vm_obj_list);
	}

	/* Finished initialising the vma, add it to task */
	dprintf("%s: Mapping 0x%x - 0x%x\n", __FUNCTION__,
		map_address, map_address + __pfn_to_addr(npages));
	task_add_vma(task, new);

	/*
	 * If area is going to be used going downwards, (i.e. as a stack)
	 * we return the *end* of the area as the start address.
	 */
	if (flags & VMA_GROWSDOWN)
		map_address += __pfn_to_addr(npages);

	return (void *)map_address;
}

/* mmap system call implementation */
int sys_mmap(l4id_t sender, void *start, size_t length, int prot,
	     int flags, int fd, unsigned long pfn)
{
	unsigned long npages = __pfn(page_align_up(length));
	unsigned long base = (unsigned long)start;
	struct vm_file *file = 0;
	unsigned int vmflags = 0;
	struct tcb *task;
	int err;

	if (!(task = find_task(sender)))
		return -ESRCH;

	/* Check fd validity */
	if (!(flags & MAP_ANONYMOUS))
		if (!task->files->fd[fd].vmfile)
			if ((err = file_open(task, fd)) < 0)
				return err;

	if (base < task->start || base >= task->end)
		return -EINVAL;

	/* Exclude task's stack, text and data from mmappable area in task's space */
	if (base < task->map_start || base >= task->map_end || !base) {
		if (flags & MAP_FIXED)	/* Its fixed, we cannot satisfy it */
			return -EINVAL;
		else
			start = 0;
	}

	/* TODO:
	 * Check that @start does not already have a mapping.
	 * Check that pfn + npages range is within the file range.
	 * Check that posix flags passed match those defined in vm_area.h
	 */
	if (flags & MAP_ANONYMOUS) {
		file = 0;
		vmflags |= VMA_ANONYMOUS;
	} else {
		file = task->files->fd[fd].vmfile;
	}

	if (flags & MAP_FIXED)
		vmflags |= VMA_FIXED;

	if (flags & MAP_PRIVATE)
		/* This means COW, if writeable. */
		vmflags |= VMA_PRIVATE;
	else	/* This also means COW, if writeable and anonymous */
		vmflags |= VMA_SHARED;

	if (flags & MAP_GROWSDOWN)
		vmflags |= VMA_GROWSDOWN;

	if (prot & PROT_READ)
		vmflags |= VM_READ;
	if (prot & PROT_WRITE)
		vmflags |= VM_WRITE;
	if (prot & PROT_EXEC)
		vmflags |= VM_EXEC;

	start = do_mmap(file, __pfn_to_addr(pfn), task, base, vmflags, npages);

	l4_ipc_return((int)start);
	return 0;
}

/* Sets the end of data segment for sender */
int sys_brk(l4id_t sender, void *ds_end)
{
	return 0;
}

