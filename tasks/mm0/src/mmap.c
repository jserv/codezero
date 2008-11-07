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
#include <memory.h>
#include <task.h>
#include <mmap.h>
#include <file.h>
#include <shm.h>
#include <syscalls.h>
#include <user.h>

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

/* Search an empty space in the task's mmapable address region. */
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
 * Returns a suitably mmap'able address. It allocates
 * differently for shared and private areas.
 */
unsigned long mmap_new_address(struct tcb *task, unsigned int flags,
			       unsigned int npages)
{
	if (flags & VMA_SHARED)
		return (unsigned long)shm_new_address(npages);
	else
		return find_unmapped_area(npages, task);
}


/*
 * Maps the given file with given flags at the given page offset to the given
 * task's address space at the specified virtual memory address and length.
 *
 * The actual paging in/out of the file from/into memory pages is handled by
 * the file's pager upon page faults.
 */
void *do_mmap(struct vm_file *mapfile, unsigned long file_offset,
	      struct tcb *task, unsigned long map_address,
	      unsigned int flags, unsigned int npages)
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
		if (flags & VMA_FIXED)
			return PTR_ERR(-EINVAL);
		else if (!(map_address = mmap_new_address(task, flags, npages)))
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
void *__sys_mmap(struct tcb *task, void *start, size_t length, int prot,
	      	 int flags, int fd, unsigned long file_offset)
{
	unsigned int vmflags = 0;
	struct vm_file *file = 0;
	int err;

	/* Check file validity */
	if (!(flags & MAP_ANONYMOUS))
		if (!task->files->fd[fd].vmfile)
			if ((err = file_open(task, fd)) < 0)
				return PTR_ERR(err);

	/* Check file offset is page aligned */
	if (!is_page_aligned(file_offset))
		return PTR_ERR(-EINVAL);

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

	return do_mmap(file, file_offset, task, (unsigned long)start,
		       vmflags, __pfn(page_align_up(length)));
}

void *sys_mmap(struct tcb *task, struct sys_mmap_args *args)
{

	struct sys_mmap_args *mapped_args;
	void *ret;

	if (!(mapped_args = pager_validate_map_user_range(task, args,
							  sizeof(*args),
							  VM_READ | VM_WRITE)))
		return PTR_ERR(-EINVAL);

	ret = __sys_mmap(task, mapped_args->start, mapped_args->length,
			 mapped_args->prot, mapped_args->flags, mapped_args->fd,
			 mapped_args->offset);

	pager_unmap_user_range(mapped_args, sizeof(*args));

	return ret;
}

/* Sets the end of data segment for sender */
int sys_brk(struct tcb *sender, void *ds_end)
{
	return 0;
}

