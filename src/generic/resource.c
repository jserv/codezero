/*
 * Initialize system resource management.
 *
 * Copyright (C) 2009 Bahadir Balban
 */

#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/generic/container.h>
#include <l4/generic/resource.h>
#include <l4/generic/bootmem.h>
#include <l4/lib/math.h>
#include <l4/lib/memcache.h>
#include INC_GLUE(memory.h)
#include INC_ARCH(linker.h)

struct kernel_container kernel_container;

void cap_list_init(struct cap_list *clist)
{
	clist->ncaps = 0;
	link_init(&clist->caps);
}

void cap_list_insert(struct capability *cap, struct cap_list *clist)
{
	list_insert(&cap->list, &clist->caps);
	clist->ncaps++;
}

/*
 * This splits a capability, splitter region must be in
 * the *middle* of original capability
 */
int memcap_split(struct capability *cap, struct cap_list *cap_list,
		 const unsigned long start,
		 const unsigned long end)
{
	struct capability *new;

	/* Allocate a capability first */
	new = alloc_bootmem(sizeof(*new), 0);

	/*
	 * Some sanity checks to show that splitter range does end up
	 * producing two smaller caps.
	 */
	BUG_ON(cap->start >= start || cap->end <= end);

	/* Update new and original caps */
	new->end = cap->end;
	new->start = end;
	cap->end = start;
	new->access = cap->access;

	/* Add new one next to original cap */
	cap_list_insert(new, cap_list);

	return 0;
}

/* This shrinks the cap from *one* end only, either start or end */
int memcap_shrink(struct capability *cap, struct cap_list *cap_list,
		  const unsigned long start, const unsigned long end)
{
	/* Shrink from the end */
	if (cap->start < start) {
		BUG_ON(start >= cap->end);
		cap->end = start;

	/* Shrink from the beginning */
	} else if (cap->end > end) {
		BUG_ON(end <= cap->start);
		cap->start = end;
	} else
		BUG();

	return 0;
}

int memcap_unmap_range(struct capability *cap,
		       struct cap_list *cap_list,
		       const unsigned long start,
		       const unsigned long end)
{
	/* Split needed? */
	if (cap->start < start && cap->end > end)
		return memcap_split(cap, cap_list, start, end);
	/* Shrink needed? */
	else if (((cap->start >= start) && (cap->end > end))
	    	   || ((cap->start < start) && (cap->end <= end)))
		return memcap_shrink(cap, cap_list, start, end);
	/* Destroy needed? */
	else if ((cap->start >= start) && (cap->end <= end))
		/* Simply unlink it */
		list_remove(&cap->list);
	else
		BUG();

	return 0;
}

/*
 * Unmaps given memory range from the list of capabilities
 * by either shrinking, splitting or destroying the
 * intersecting capability. Similar to do_munmap()
 */
int memcap_unmap(struct cap_list *cap_list,
		 const unsigned long unmap_start,
		 const unsigned long unmap_end)
{
	struct capability *cap, *n;
	int err;

	list_foreach_removable_struct(cap, n, &cap_list->caps, list) {
		/* Check for intersection */
		if (set_intersection(unmap_start, unmap_end,
				     cap->start, cap->end)) {
			if ((err = memcap_unmap_range(cap, cap_list,
						      unmap_start,
						      unmap_end))) {
				return err;
			}
			return 0;
		}
	}
	/* Return 1 to indicate unmap didn't occur */
	return 1;
}

/*
 * Do all system accounting for this capability info
 * structure that belongs to a container, such as
 * count its resource requirements, remove its portion
 * from global kernel capabilities etc.
 */
int process_cap_info(struct cap_info *cap,
		     struct boot_resources *bootres,
		     struct kernel_container *kcont)
{
	int ret;

	switch (cap->type & CAP_RTYPE_MASK) {
	case CAP_RTYPE_THREADPOOL:
		bootres->nthreads += cap->size;
		break;
	case CAP_RTYPE_SPACEPOOL:
		bootres->nspaces += cap->size;
		break;
	case CAP_RTYPE_MUTEXPOOL:
		bootres->nmutex += cap->size;
		break;
	case CAP_RTYPE_VIRTMEM:
		/* Area size in pages divided by mapsize in pages */
		bootres->npmds +=
			cap->size / __pfn(PMD_MAP_SIZE);
		if ((ret = memcap_unmap(&kcont->virtmem_free,
			     		cap->start, cap->end))) {
			if (ret < 0)
				printk("FATAL: Insufficient boot memory "
				       "to split capability\n");
			if (ret > 0)
				printk("FATAL: Memory capability range "
				       "overlaps with another one. "
				       "start=0x%lx, end=0x%lx\n",
				       __pfn_to_addr(cap->start),
				       __pfn_to_addr(cap->end));
			BUG();
		}
		break;
	case CAP_RTYPE_PHYSMEM:
		if ((ret = memcap_unmap(&kcont->virtmem_free,
					cap->start, cap->end))) {
			if (ret < 0)
				printk("FATAL: Insufficient boot memory "
				       "to split capability\n");
			if (ret > 0)
				printk("FATAL: Memory capability range "
				       "overlaps with another one. "
				       "start=0x%lx, end=0x%lx\n",
				       __pfn_to_addr(cap->start),
				       __pfn_to_addr(cap->end));
			BUG();
		}
		break;
	}
	return ret;
}

/*
 * Migrate any boot allocations to their relevant caches.
 */
void migrate_boot_resources(struct boot_resources *bootres,
			    struct kernel_container *kcont)
{
	/* Migrate boot page tables to new caches */
	//	migrate_page_tables(kcont);

	/* Migrate all boot-allocated capabilities */
	// migrate_boot_caps(kcont);
}

/* Delete all boot memory and add it to physical memory pool. */
int free_boot_memory(struct boot_resources *bootres,
		     struct kernel_container *kcont)
{
	/* Trim kernel used memory cap */
	memcap_unmap(&kcont->physmem_used, (unsigned long)_start_init,
		     (unsigned long)_end_init);

	/* Add it to unused physical memory */
	// memcap_map(&kcont->physmem_free, (unsigned long)_start_init,
	//	(unsigned long)_end_init);

       /*
	* Freed physical area will be unmapped from virtual
	* by not mapping it in the task page tables.
	*/
	return 0;
}


struct mem_cache *init_resource_cache(int nstruct, int struct_size,
				      struct kernel_container *kcont,
				      int aligned)
{
	struct capability *cap;
	unsigned long bufsize;

	/* In all unused physical memory regions */
	list_foreach_struct(cap, &kcont->physmem_free.caps, list) {
		/* Get buffer size needed for cache */
		bufsize = mem_cache_bufsize((void *)__pfn_to_addr(cap->start),
					    struct_size, nstruct,
					    aligned);
		/*
		 * Check if memcap region size is enough to cover
		 * resource allocation
		 */
		if (__pfn_to_addr(cap->end - cap->start) >= bufsize) {
			unsigned long virtual =
				phys_to_virt(__pfn_to_addr(cap->start));
			/*
			 * Map the buffer as boot mapping if pmd caches
			 * are not initialized
			 */
			if (!kcont->pmd_cache) {
				add_boot_mapping(__pfn_to_addr(cap->start),
						 virtual, bufsize,
						 MAP_SVC_RW_FLAGS);
			} else {
				add_mapping(__pfn_to_addr(cap->start),
					    virtual, bufsize,
					    MAP_SVC_RW_FLAGS);
			}
			/* Unmap area from memcap */
			memcap_unmap_range(cap, &kcont->physmem_free,
					   cap->start, cap->start +
					   __pfn(page_align_up((bufsize))));

			/* TODO: Manipulate memcaps for virtual range??? */

			/* Initialize the cache */
			return mem_cache_init((void *)virtual, bufsize,
					      PGD_SIZE, 1);
		}
	}
	return 0;
}

/*
 * Initializes kernel caplists, and sets up total of physical
 * and virtual memory as single capabilities of the kernel.
 * They will then get split into caps of different lengths
 * during the traversal of container capabilities.
 */
void init_kernel_container(struct kernel_container *kcont)
{
	struct capability *physmem, *virtmem, *kernel_area;

	/* Initialize kernel capability lists */
	cap_list_init(&kcont->physmem_used);
	cap_list_init(&kcont->physmem_free);
	cap_list_init(&kcont->virtmem_used);
	cap_list_init(&kcont->virtmem_free);
	cap_list_init(&kcont->devmem_used);
	cap_list_init(&kcont->devmem_free);

	/* Set up total physical memory as single capability */
	physmem = alloc_bootmem(sizeof(*physmem), 0);
	physmem->start = __pfn(PHYS_MEM_START);
	physmem->end = __pfn(PHYS_MEM_END);
	link_init(&physmem->list);
	cap_list_insert(physmem, &kcont->physmem_free);

	/* Set up total virtual memory as single capability */
	virtmem = alloc_bootmem(sizeof(*virtmem), 0);
	virtmem->start = __pfn(VIRT_MEM_START);
	virtmem->end = __pfn(VIRT_MEM_END);
	link_init(&virtmem->list);
	cap_list_insert(virtmem, &kcont->virtmem_free);

	/* Set up kernel used area as a single capability */
	kernel_area = alloc_bootmem(sizeof(*physmem), 0);
	kernel_area->start = __pfn(virt_to_phys(_start_kernel));
	kernel_area->end = __pfn(virt_to_phys(_end_kernel));
	link_init(&kernel_area->list);
	cap_list_insert(kernel_area, &kcont->physmem_used);

	/* Unmap kernel used area from free physical memory capabilities */
	memcap_unmap(&kcont->physmem_free, kernel_area->start,
		     kernel_area->end);

	/* TODO:
	 * Add all virtual memory areas used by the kernel
	 * e.g. kernel virtual area, syscall page, kip page,
	 * vectors page, timer, sysctl and uart device pages
	 */
}

void create_containers(struct boot_resources *bootres,
		       struct kernel_container *kcont)
{

}

void create_capabilities(struct boot_resources *bootres,
		         struct kernel_container *kcont)
{

}

/*
 * Make sure to count boot pmds, and kernel capabilities
 * created in boot memory.
 *
 * Also total capabilities in the system + number of
 * capabilities containers are allowed to create dynamically.
 *
 * Count the extra pgd + space needed in case all containers quit
 */
void init_resource_allocators(struct boot_resources *bootres,
			      struct kernel_container *kcont)
{
	/* Initialise PGD cache */
	kcont->pgd_cache = init_resource_cache(bootres->nspaces,
				    	       PGD_SIZE, kcont, 1);

	/* Initialise PMD cache */
	kcont->pmd_cache = init_resource_cache(bootres->npmds,
					       PMD_SIZE, kcont, 1);

	/* Initialise struct address_space cache */
	kcont->address_space_cache =
		init_resource_cache(bootres->nspaces,
				    sizeof(struct address_space),
				    kcont, 0);

	/* Initialise ktcb cache */
	kcont->ktcb_cache = init_resource_cache(bootres->nthreads,
						PAGE_SIZE, kcont, 1);

	/* Initialise umutex cache */
	kcont->mutex_cache = init_resource_cache(bootres->nmutex,
						 sizeof(struct mutex_queue),
						 kcont, 0);
	/* TODO: Initialize ID cache */

	/* Initialise capability cache */
	kcont->cap_cache = init_resource_cache(bootres->ncaps, /* FIXME: Count correctly */
					       sizeof(struct capability),
					       kcont, 0);
	/* Initialise container cache */
	kcont->cont_cache = init_resource_cache(bootres->nconts,
						sizeof(struct container),
						kcont, 0);

	/* Create system containers */
	create_containers(bootres, kcont);

	/* Create capabilities */
	create_capabilities(bootres, kcont);
}

int init_boot_resources(struct boot_resources *bootres,
			struct kernel_container *kcont)
{
	struct cap_info *cap;

	init_kernel_container(kcont);

	/* Number of containers known at compile-time */
	bootres->nconts = TOTAL_CONTAINERS;

	/* Traverse all containers */
	for (int i = 0; i < bootres->nconts; i++) {
		/* Traverse all pagers */
		for (int j = 0; j < cinfo[i].npagers; j++) {
			int ncaps = cinfo[i].pager[j].ncaps;

			/* Count all capabilities */
			bootres->ncaps += ncaps;

			/* Count all resources */
			for (int k = 0; k < ncaps; k++) {
				cap = &cinfo[i].pager[j].caps[k];
				process_cap_info(cap, bootres, kcont);
			}
		}
	}

	/* TODO: Count all ids needed to represent all */
	return 0;
}

/*
 * FIXME: Add error handling
 */
int init_system_resources(struct kernel_container *kcont)
{

	struct boot_resources bootres;

	memset(&bootres, 0, sizeof(bootres));

	init_boot_resources(&bootres, kcont);

	init_resource_allocators(&bootres, kcont);

	free_boot_memory(&bootres, kcont);

	return 0;
}




