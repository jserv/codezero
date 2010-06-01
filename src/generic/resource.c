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
#include <l4/generic/platform.h>
#include <l4/lib/math.h>
#include <l4/lib/memcache.h>
#include INC_GLUE(memory.h)
#include INC_GLUE(mapping.h)
#include INC_ARCH(linker.h)
#include INC_PLAT(platform.h)
#include <l4/api/errno.h>
#include <l4/generic/idle.h>

struct kernel_resources kernel_resources;

pgd_table_t *pgd_alloc(void)
{
	return mem_cache_zalloc(kernel_resources.pgd_cache);
}

pmd_table_t *pmd_cap_alloc(struct cap_list *clist)
{
	struct capability *cap;

	if (!(cap = cap_list_find_by_rtype(clist,
					   CAP_RTYPE_MAPPOOL)))
		return 0;

	if (capability_consume(cap, 1) < 0)
		return 0;

	return mem_cache_zalloc(kernel_resources.pmd_cache);
}

struct address_space *space_cap_alloc(struct cap_list *clist)
{
	struct capability *cap;

	if (!(cap = cap_list_find_by_rtype(clist,
					   CAP_RTYPE_SPACEPOOL)))
		return 0;

	if (capability_consume(cap, 1) < 0)
		return 0;

	return mem_cache_zalloc(kernel_resources.space_cache);
}

struct ktcb *ktcb_cap_alloc(struct cap_list *clist)
{
	struct capability *cap;

	if (!(cap = cap_list_find_by_rtype(clist,
					   CAP_RTYPE_THREADPOOL)))
		return 0;

	if (capability_consume(cap, 1) < 0)
		return 0;

	return mem_cache_zalloc(kernel_resources.ktcb_cache);
}

struct mutex_queue *mutex_cap_alloc()
{
	struct capability *cap;

	if (!(cap = cap_find_by_rtype(current,
				      CAP_RTYPE_MUTEXPOOL)))
		return 0;

	if (capability_consume(cap, 1) < 0)
		return 0;

	return mem_cache_zalloc(kernel_resources.mutex_cache);
}

void pgd_free(void *addr)
{
	BUG_ON(mem_cache_free(kernel_resources.pgd_cache, addr) < 0);
}

void pmd_cap_free(void *addr, struct cap_list *clist)
{
	struct capability *cap;

	BUG_ON(!(cap = cap_list_find_by_rtype(clist,
					      CAP_RTYPE_MAPPOOL)));
	capability_free(cap, 1);

	BUG_ON(mem_cache_free(kernel_resources.pmd_cache, addr) < 0);
}

void space_cap_free(void *addr, struct cap_list *clist)
{
	struct capability *cap;

	BUG_ON(!(cap = cap_list_find_by_rtype(clist,
					      CAP_RTYPE_SPACEPOOL)));
	capability_free(cap, 1);

	BUG_ON(mem_cache_free(kernel_resources.space_cache, addr) < 0);
}


/*
 * Account it to pager, but if it doesn't exist,
 * to current idle task
 */
void ktcb_cap_free(void *addr, struct cap_list *clist)
{
	struct capability *cap;

	/* Account it to task's pager if it exists */
	BUG_ON(!(cap = cap_list_find_by_rtype(clist,
					      CAP_RTYPE_THREADPOOL)));
	capability_free(cap, 1);

	BUG_ON(mem_cache_free(kernel_resources.ktcb_cache, addr) < 0);
}

void mutex_cap_free(void *addr)
{
	struct capability *cap;

	BUG_ON(!(cap = cap_find_by_rtype(current,
					 CAP_RTYPE_MUTEXPOOL)));
	capability_free(cap, 1);

	BUG_ON(mem_cache_free(kernel_resources.mutex_cache, addr) < 0);
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

/*
 * Given a single memory cap (that definitely overlaps) removes
 * the portion of pfns specified by start/end.
 */
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
int memcap_unmap(struct cap_list *used_list,
		 struct cap_list *cap_list,
		 const unsigned long unmap_start,
		 const unsigned long unmap_end)
{
	struct capability *cap, *n;
	int ret;

	/*
	 * If a used list was supplied, check that the
	 * range does not intersect with the used list.
	 * This is an optional sanity check.
	 */
	if (used_list) {
		list_foreach_removable_struct(cap, n,
					      &used_list->caps,
					      list) {
			if (set_intersection(unmap_start, unmap_end,
					     cap->start, cap->end)) {
				ret = -EPERM;
				goto out_err;
			}
		}
	}

	list_foreach_removable_struct(cap, n, &cap_list->caps, list) {
		/* Check for intersection */
		if (set_intersection(unmap_start, unmap_end,
				     cap->start, cap->end)) {
			if ((ret = memcap_unmap_range(cap, cap_list,
						      unmap_start,
						      unmap_end))) {
				goto out_err;
			}
			return 0;
		}
	}
	return -EEXIST;

out_err:
	if (ret == -ENOMEM)
		printk("%s: FATAL: Insufficient boot memory "
		       "to split capability\n", __KERNELNAME__);
	else if (ret == -EPERM)
		printk("%s: FATAL: %s memory capability range "
		       "overlaps with an already used range. "
		       "start=0x%lx, end=0x%lx\n", __KERNELNAME__,
		       cap_type(cap) == CAP_TYPE_MAP_VIRTMEM ?
		       "Virtual" : "Physical",
		       __pfn_to_addr(cap->start),
		       __pfn_to_addr(cap->end));
	BUG();
}

/*
 * Copies cap_info structures to real capabilities in a list
 */
void copy_cap_info(struct cap_list *clist, struct cap_info *cap_info, int ncaps)
{
	struct capability *cap;
	struct cap_info *cinfo;

	for (int i = 0; i < ncaps; i++) {
		cinfo = &cap_info[i];
		cap = alloc_bootmem(sizeof(*cap), 0);

		cap->resid = cinfo->target;
		cap->type = cinfo->type;
		cap->access = cinfo->access;
		cap->start = cinfo->start;
		cap->end = cinfo->end;
		cap->size = cinfo->size;

		cap_list_insert(cap, clist);
	}
}

/*
 * Copies cinfo structures to real capabilities for each pager.
 */
int copy_pager_info(struct pager *pager, struct pager_info *pinfo)
{

	pager->start_address = pinfo->start_address;
	pager->start_lma = __pfn_to_addr(pinfo->pager_lma);
	pager->start_vma = __pfn_to_addr(pinfo->pager_vma);
	pager->memsize = __pfn_to_addr(pinfo->pager_size);
	pager->rw_pheader_start = pinfo->rw_pheader_start;
	pager->rw_pheader_end = pinfo->rw_pheader_end;
	pager->rx_pheader_start = pinfo->rx_pheader_start;
	pager->rx_pheader_end = pinfo->rx_pheader_end;

	copy_cap_info(&pager->cap_list, pinfo->caps, pinfo->ncaps);

	return 0;
}

/*
 * Copies container info from a given compact container descriptor to
 * a real container
 */
int copy_container_info(struct container *c, struct container_info *cinfo)
{
	strncpy(c->name, cinfo->name, CONFIG_CONTAINER_NAMESIZE);
	c->npagers = cinfo->npagers;

	/* Copy container capabilities */
	copy_cap_info(&c->cap_list, cinfo->caps, cinfo->ncaps);

	/* Copy pager capabilities and boot info */
	for (int i = 0; i < c->npagers; i++)
		copy_pager_info(&c->pager[i], &cinfo->pager[i]);

	return 0;
}

/*
 * Given a structure size and numbers, it initializes a memory cache
 * using free memory available from free kernel memory capabilities.
 */
struct mem_cache *init_resource_cache(int nstruct, int struct_size,
				      struct kernel_resources *kres,
				      int aligned)
{
	struct capability *cap;
	unsigned long bufsize;

	/* In all unused physical memory regions */
	list_foreach_struct(cap, &kres->physmem_free.caps, list) {
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
			if (!kres->pmd_cache) {
				add_boot_mapping(__pfn_to_addr(cap->start),
						 virtual,
						 page_align_up(bufsize),
						 MAP_KERN_RW);
			} else {
				add_mapping_space(__pfn_to_addr(cap->start),
						  virtual, page_align_up(bufsize),
						  MAP_KERN_RW, current->space);
			}
			/* Unmap area from memcap */
			memcap_unmap_range(cap, &kres->physmem_free,
					   cap->start, cap->start +
					   __pfn(page_align_up((bufsize))));

			/* TODO: Manipulate memcaps for virtual range??? */

			/* Initialize the cache */
			return mem_cache_init((void *)virtual, bufsize,
					      struct_size, aligned);
		}
	}
	return 0;
}

/*
 * TODO: Initialize ID cache
 *
 * Given a kernel resources and the set of boot resources required,
 * initializes all memory caches for allocations. Once caches are
 * initialized, earlier boot allocations are migrated to caches.
 */
void init_resource_allocators(struct boot_resources *bootres,
			      struct kernel_resources *kres)
{
	/* Initialise PGD cache */
	kres->pgd_cache =
		init_resource_cache(bootres->nspaces,
				    PGD_SIZE, kres, 1);

	/* Initialise address space cache */
	kres->space_cache =
		init_resource_cache(bootres->nspaces,
				    sizeof(struct address_space),
				    kres, 0);

	/* Initialise ktcb cache */
	kres->ktcb_cache =
		init_resource_cache(bootres->nthreads,
				    PAGE_SIZE, kres, 1);

	/* Initialise umutex cache */
	kres->mutex_cache =
		init_resource_cache(bootres->nmutex,
				    sizeof(struct mutex_queue),
				    kres, 0);

	/* Initialise PMD cache */
	kres->pmd_cache =
		init_resource_cache(bootres->npmds,
				    PMD_SIZE, kres, 1);
}

/*
 * Do all system accounting for a given capability info
 * structure that belongs to a container, such as
 * count its resource requirements, remove its portion
 * from global kernel resource capabilities etc.
 */
int process_cap_info(struct cap_info *cap,
		     struct boot_resources *bootres,
		     struct kernel_resources *kres)
{
	int ret = 0;

	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREADPOOL:
		bootres->nthreads += cap->size;
		break;

	case CAP_RTYPE_SPACEPOOL:
		bootres->nspaces += cap->size;
		break;

	case CAP_RTYPE_MUTEXPOOL:
		bootres->nmutex += cap->size;
		break;

	case CAP_RTYPE_MAPPOOL:
		/* Speficies how many pmds can be mapped */
		bootres->npmds += cap->size;
		break;
	}

	if (cap_type(cap) == CAP_TYPE_MAP_VIRTMEM) {
		memcap_unmap(&kres->virtmem_used,
			     &kres->virtmem_free,
			     cap->start, cap->end);
	} else if (cap_type(cap) == CAP_TYPE_MAP_PHYSMEM) {
		/* Try physical ram ranges */
		if ((ret = memcap_unmap(&kres->physmem_used,
					&kres->physmem_free,
					cap->start, cap->end))
		    == -EEXIST) {
			/* Try physical device ranges */
			if ((ret = memcap_unmap(&kres->devmem_used,
					&kres->devmem_free,
					cap->start, cap->end))
			    == -EEXIST) {
				/* Neither is a match */
				printk("%s: FATAL: Physical memory "
				       "capability range does not match "
				       "with any available physmem or devmem "
				       "free range. start=0x%lx, end=0x%lx\n",
				       __KERNELNAME__,
				       __pfn_to_addr(cap->start),
				       __pfn_to_addr(cap->end));
				BUG();
			}
		}
	}
	return ret;
}

/*
 * Initialize kernel managed physical memory capabilities
 * based on the multiple physical memory and device regions
 * defined by the specific platform.
 */
void kernel_setup_physmem(struct kernel_resources *kres,
			  struct platform_mem_regions *mem_regions)
{
	struct capability *physmem;

	for (int i = 0; i < mem_regions->nregions; i++) {
		/* Allocate new physical memory capability */
		physmem = alloc_bootmem(sizeof(*physmem), 0);

		/* Assign its range from platform range at this index */
		physmem->start = __pfn(mem_regions->mem_range[i].start);
		physmem->end = __pfn(mem_regions->mem_range[i].end);

		/* Init and insert the capability to keep permanently */
		link_init(&physmem->list);
		if (mem_regions->mem_range[i].type == MEM_TYPE_RAM)
			cap_list_insert(physmem, &kres->physmem_free);
		else if (mem_regions->mem_range[i].type == MEM_TYPE_DEV)
			cap_list_insert(physmem, &kres->devmem_free);
		else BUG();
	}
}

/*
 * Initializes all system resources and handling of those
 * resources. First descriptions are done by allocating from
 * boot memory, once memory caches are initialized, boot
 * memory allocations are migrated over to caches.
 */
int init_system_resources(struct kernel_resources *kres)
{
	struct cap_info *cap_info;
	// struct capability *cap;
	struct container *container;
	struct capability *virtmem, *kernel_area;
	struct boot_resources bootres;

	/* Initialize system id pools */
	kres->space_ids.nwords = SYSTEM_IDS_MAX;
	kres->ktcb_ids.nwords = SYSTEM_IDS_MAX;
	kres->resource_ids.nwords = SYSTEM_IDS_MAX;
	kres->container_ids.nwords = SYSTEM_IDS_MAX;
	kres->mutex_ids.nwords = SYSTEM_IDS_MAX;
	kres->capability_ids.nwords = SYSTEM_IDS_MAX;

	/* Initialize container head */
	container_head_init(&kres->containers);

	/* Initialize kernel capability lists */
	cap_list_init(&kres->physmem_used);
	cap_list_init(&kres->physmem_free);
	cap_list_init(&kres->devmem_used);
	cap_list_init(&kres->devmem_free);
	cap_list_init(&kres->virtmem_used);
	cap_list_init(&kres->virtmem_free);

	/* Initialize kernel address space */
	link_init(&kres->init_space.list);
	cap_list_init(&kres->init_space.cap_list);
	spin_lock_init(&kres->init_space.lock);

	// Shall we have this?
	// space->spid = id_new(&kernel_resources.space_ids);

	/*
	 * Set up total physical memory capabilities as many
	 * as the platform-defined physical memory regions
	 */
	kernel_setup_physmem(kres, &platform_mem_regions);

	/* Set up total virtual memory as single capability */
	virtmem = alloc_bootmem(sizeof(*virtmem), 0);
	virtmem->start = __pfn(VIRT_MEM_START);
	virtmem->end = __pfn(VIRT_MEM_END);
	link_init(&virtmem->list);
	cap_list_insert(virtmem, &kres->virtmem_free);

	/* Set up kernel used area as a single capability */
	kernel_area = alloc_bootmem(sizeof(*kernel_area), 0);
	kernel_area->start = __pfn(virt_to_phys(_start_kernel));
	kernel_area->end = __pfn(virt_to_phys(_end_kernel));
	link_init(&kernel_area->list);
	cap_list_insert(kernel_area, &kres->physmem_used);

	/* Unmap kernel used area from free physical memory capabilities */
	memcap_unmap(0, &kres->physmem_free, kernel_area->start,
		     kernel_area->end);

	/* TODO:
	 * Add all virtual memory areas used by the kernel
	 * e.g. kernel virtual area, syscall page, kip page,
	 */

	memset(&bootres, 0, sizeof(bootres));

	/* Number of containers known at compile-time */
	bootres.nconts = CONFIG_CONTAINERS;

	/* Traverse all containers */
	for (int i = 0; i < bootres.nconts; i++) {

		/* Process container-wide capabilities */
		bootres.ncaps += cinfo[i].ncaps;
		for (int g = 0; g < cinfo[i].ncaps; g++) {
			cap_info = &cinfo[i].caps[g];
			process_cap_info(cap_info, &bootres, kres);
		}

		/* Traverse all pagers */
		for (int j = 0; j < cinfo[i].npagers; j++) {
			int ncaps = cinfo[i].pager[j].ncaps;

			/* Count all capabilities */
			bootres.ncaps += ncaps;

			/*
			 * Count all resources,
			 *
			 * Remove all container memory resources
			 * from global.
			 */
			for (int k = 0; k < ncaps; k++) {
				cap_info = &cinfo[i].pager[j].caps[k];
				process_cap_info(cap_info, &bootres, kres);
			}
		}
	}

	init_resource_allocators(&bootres, kres);

	/*
	 * Setting up ids used internally.
	 *
	 * See how many containers we have. Assign next
	 * unused container id for kernel resources
	 */
	kres->cid = id_get(&kres->container_ids, bootres.nconts + 1);
	// kres->cid = id_get(&kres->container_ids, 0); // Gets id 0

	/*
	 * Assign thread and space ids to current which will later
	 * become the idle task
	 * TODO: Move this to idle task initialization?
	 */
	current->tid = id_new(&kres->ktcb_ids);
	current->space->spid = id_new(&kres->space_ids);

	/*
	 * Init per-cpu zombie lists
	 */
	for (int i = 0; i < CONFIG_NCPU; i++)
		init_ktcb_list(&per_cpu_byid(kres->zombie_list, i));

	/*
	 * Create real containers from compile-time created
	 * cinfo structures
	 */
	for (int i = 0; i < bootres.nconts; i++) {
		/* Allocate & init container */
		container = container_alloc_init();

		/* Fill in its information */
		copy_container_info(container, &cinfo[i]);

		/* Add it to kernel resources list */
		kres_insert_container(container, kres);
	}

	/* Initialize pagers */
	container_init_pagers(kres);

	return 0;
}

