/*
 * Functions to validate, map and unmap user buffers.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <l4lib/arch/syslib.h>
#include <vm_area.h>
#include <task.h>
#include <user.h>

/*
 * Checks if the given user virtual address range is
 * validly owned by that user with given flags.
 *
 * FIXME: This scans the vmas page by page, we can do it faster
 * by leaping from one vma to next.
 */
int pager_validate_user_range(struct tcb *user, void *userptr, unsigned long size,
			      unsigned int vmflags)
{
	struct vm_area *vma;
	unsigned long start = page_align(userptr);
	unsigned long end = page_align_up(userptr + size);

	/* Find the vma that maps that virtual address */
	for (unsigned long vaddr = start; vaddr < end; vaddr += PAGE_SIZE) {
		if (!(vma = find_vma(vaddr, &user->vm_area_head->list))) {
			//printf("%s: No VMA found for 0x%x on task: %d\n",
			//       __FUNCTION__, vaddr, user->tid);
			return -1;
		}
		if ((vma->flags & vmflags) != vmflags)
			return -1;
	}

	return 0;
}

/*
 * Validates and maps the user virtual address range to the pager.
 * Every virtual page needs to be mapped individually because it's
 * not guaranteed pages are physically contiguous.
 *
 * FIXME: There's no logic here to make non-contiguous physical pages
 * to get mapped virtually contiguous.
 */
void *pager_validate_map_user_range(struct tcb *user, void *userptr,
				    unsigned long size, unsigned int vm_flags)
{
	unsigned long start = page_align(userptr);
	unsigned long end = page_align_up(userptr + size);
	void *mapped = 0;

	/* Validate that user task owns this address range */
	if (pager_validate_user_range(user, userptr, size, vm_flags) < 0)
		return 0;

	/* Map first page and calculate the mapped address of pointer */
	mapped = l4_map_helper((void *)page_to_phys(task_virt_to_page(user, start)), 1);
	mapped = (void *)(((unsigned long)mapped) |
			  ((unsigned long)(PAGE_MASK & (unsigned long)userptr)));

	/* Map the rest of the pages, if any */
	for (unsigned long i = start + PAGE_SIZE; i < end; i += PAGE_SIZE)
		l4_map_helper((void *)page_to_phys(task_virt_to_page(user,
							     start + i)), 1);

	return mapped;
}

void pager_unmap_user_range(void *mapped_ptr, unsigned long size)
{
	l4_unmap_helper((void *)page_align(mapped_ptr),
			__pfn(page_align_up(size)));
}

