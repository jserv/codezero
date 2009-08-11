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

pgd_table_t *alloc_pgd(void)
{
	return mem_cache_zalloc(kernel_container.pgd_cache);
}

pmd_table_t *alloc_pmd(void)
{
	return mem_cache_zalloc(kernel_container.pmd_cache);
}

struct address_space *alloc_space(void)
{
	return mem_cache_zalloc(kernel_container.space_cache);
}

struct ktcb *alloc_ktcb(void)
{
	return mem_cache_zalloc(kernel_container.ktcb_cache);
}

struct capability *alloc_capability(void)
{
	return mem_cache_zalloc(kernel_container.cap_cache);
}

struct container *alloc_container(void)
{
	return mem_cache_zalloc(kernel_container.cont_cache);
}

struct mutex_queue *alloc_user_mutex(void)
{
	return mem_cache_zalloc(kernel_container.mutex_cache);
}

void free_pgd(void *addr)
{
	BUG_ON(mem_cache_free(kernel_container.pgd_cache, addr) < 0);
}

void free_pmd(void *addr)
{
	BUG_ON(mem_cache_free(kernel_container.pmd_cache, addr) < 0);
}

void free_space(void *addr)
{
	BUG_ON(mem_cache_free(kernel_container.space_cache, addr) < 0);
}

void free_ktcb(void *addr)
{
	BUG_ON(mem_cache_free(kernel_container.ktcb_cache, addr) < 0);
}

void free_capability(void *addr)
{
	BUG_ON(mem_cache_free(kernel_container.cap_cache, addr) < 0);
}

void free_container(void *addr)
{
	BUG_ON(mem_cache_free(kernel_container.cont_cache, addr) < 0);
}

void free_user_mutex(void *addr)
{
	BUG_ON(mem_cache_free(kernel_container.mutex_cache, addr) < 0);
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
 * TODO: Evaluate if access bits are needed and add new cap ranges
 * only if their access bits match.
 *
 * Maps a memory range as a capability to a list of capabilities either by
 * merging the given range to an existing capability or creating a new one.
 */
int memcap_map(struct cap_list *cap_list,
	       const unsigned long map_start,
	       const unsigned long map_end)
{
	struct capability *cap, *n;

	list_foreach_removable_struct(cap, n, &cap_list->caps, list) {
		if (cap->start == map_end) {
			cap->start = map_start;
			return 0;
		} else if(cap->end == map_start) {
			cap->end = map_end;
			return 0;
		}
	}

	/* No capability could be extended, we create a new one */
	cap = alloc_capability();
	cap->start = map_start;
	cap->end = map_end;
	link_init(&cap->list);
	cap_list_insert(cap, cap_list);

	return 0;
}

/* Delete all boot memory and add it to physical memory pool. */
int free_boot_memory(struct kernel_container *kcont)
{
	unsigned long pfn_start =
		__pfn(virt_to_phys(_start_init));
	unsigned long pfn_end =
		__pfn(page_align_up(virt_to_phys(_end_init)));

	/* Trim kernel used memory cap */
	memcap_unmap(&kcont->physmem_used, pfn_start, pfn_end);

	/* Add it to unused physical memory */
	memcap_map(&kcont->physmem_free, pfn_start, pfn_end);

	/* Remove the init memory from the page tables */
	for (unsigned long i = pfn_start; i < pfn_end; i++)
		remove_mapping(phys_to_virt(__pfn_to_addr(i)));

	return 0;
}

/*
 * Initializes kernel caplists, and sets up total of physical
 * and virtual memory as single capabilities of the kernel.
 * They will then get split into caps of different lengths
 * during the traversal of container capabilities, and memcache
 * allocations.
 */
void init_kernel_container(struct kernel_container *kcont)
{
	struct capability *physmem, *virtmem, *kernel_area;

	/* Initialize system id pools */
	kcont->space_ids.nwords = SYSTEM_IDS_MAX;
	kcont->ktcb_ids.nwords = SYSTEM_IDS_MAX;
	kcont->resource_ids.nwords = SYSTEM_IDS_MAX;
	kcont->container_ids.nwords = SYSTEM_IDS_MAX;
	kcont->mutex_ids.nwords = SYSTEM_IDS_MAX;
	kcont->capability_ids.nwords = SYSTEM_IDS_MAX;

	/* Initialize container head */
	container_head_init(&kcont->containers);

	/* Get first container id for itself */
	kcont->cid = id_new(&kcont->container_ids);

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

/*
 * Copies cinfo structures to real capabilities for each pager.
 *
 * FIXME: Check if pager has enough resources to create its caps.
 */
int copy_pager_info(struct pager *pager, struct pager_info *pinfo)
{
	struct capability *cap;
	struct cap_info *cap_info;

	pager->start_lma = __pfn_to_addr(pinfo->pager_lma);
	pager->start_vma = __pfn_to_addr(pinfo->pager_vma);
	pager->memsize = __pfn_to_addr(pinfo->pager_size);

	/* Copy all cinfo structures into real capabilities */
	for (int i = 0; i < pinfo->ncaps; i++) {
		cap = capability_create();

		cap_info = &pinfo->caps[i];

		cap->type = cap_info->type;
		cap->access = cap_info->access;
		cap->start = cap_info->start;
		cap->end = cap_info->end;
		cap->size = cap_info->size;

		cap_list_insert(cap, &pager->cap_list);
	}
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

	/* Copy capabilities */
	for (int i = 0; i < c->npagers; i++)
		copy_pager_info(&c->pager[i], &cinfo->pager[i]);
	return 0;
}

/*
 * Create real containers from compile-time created cinfo structures
 */
void setup_containers(struct boot_resources *bootres,
		      struct kernel_container *kcont)
{
	struct container *container;
	pgd_table_t *current_pgd;

	/*
	 * Move to real page tables, accounted by
	 * pgds and pmds provided from the caches
	 *
	 * We do not want to delay this too much,
	 * since we want to avoid allocating an uncertain
	 * amount of memory from the boot allocators.
	 */
	current_pgd = realloc_page_tables();

	/* Create all containers but leave pagers */
	for (int i = 0; i < bootres->nconts; i++) {
		/* Allocate & init container */
		container = container_create();

		/* Fill in its information */
		copy_container_info(container, &cinfo[i]);

		/* Add it to kernel container list */
		kcont_insert_container(container, kcont);
	}

	/* Initialize pagers */
	container_init_pagers(kcont, current_pgd);
}

/*
 * Copy boot-time allocated kernel capabilities to ones that
 * are allocated from the capability memcache
 */
void copy_boot_capabilities(struct cap_list *caplist)
{
	struct capability *bootcap, *n, *realcap;

	/* For every bootmem-allocated capability */
	list_foreach_removable_struct(bootcap, n,
				      &caplist->caps,
				      list) {
		/* Create new one from capability cache */
		realcap = capability_create();

		/* Copy all fields except id to real */
		realcap->owner = bootcap->owner;
		realcap->resid = bootcap->resid;
		realcap->type = bootcap->type;
		realcap->access = bootcap->access;
		realcap->start = bootcap->start;
		realcap->end = bootcap->end;

		/* Unlink boot one */
		list_remove(&bootcap->list);

		/* Add real one to head */
		list_insert(&realcap->list,
			    &caplist->caps);
	}
}

/*
 * Creates capabilities allocated with a real id, and from the
 * capability cache, in place of ones allocated at boot-time.
 */
void kcont_setup_capabilities(struct boot_resources *bootres,
			      struct kernel_container *kcont)
{
	copy_boot_capabilities(&kcont->physmem_used);
	copy_boot_capabilities(&kcont->physmem_free);
	copy_boot_capabilities(&kcont->virtmem_used);
	copy_boot_capabilities(&kcont->virtmem_free);
	copy_boot_capabilities(&kcont->devmem_used);
	copy_boot_capabilities(&kcont->devmem_free);
}

/*
 * Given a structure size and numbers, it initializes a memory cache
 * using free memory available from free kernel memory capabilities.
 */
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
						 virtual,
						 page_align_up(bufsize),
						 MAP_SVC_RW_FLAGS);
			} else {
				add_mapping_pgd(__pfn_to_addr(cap->start),
						virtual, page_align_up(bufsize),
						MAP_SVC_RW_FLAGS, &init_pgd);
			}
			/* Unmap area from memcap */
			memcap_unmap_range(cap, &kcont->physmem_free,
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
 * Given a kernel container and the set of boot resources required,
 * initializes all memory caches for allocations. Once caches are
 * initialized, earlier boot allocations are migrated to caches.
 */
void init_resource_allocators(struct boot_resources *bootres,
			      struct kernel_container *kcont)
{
	/*
	 * An extra space reserved for kernel
	 * in case all containers quit
	 */
	bootres->nspaces++;

	/* Initialise PGD cache */
	kcont->pgd_cache = init_resource_cache(bootres->nspaces,
				    	       PGD_SIZE, kcont, 1);

	/* Initialise struct address_space cache */
	kcont->space_cache =
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
	/* Initialise container cache */
	kcont->cont_cache = init_resource_cache(bootres->nconts,
						sizeof(struct container),
						kcont, 0);

	/*
	 * Add all caps used by the kernel + two extra in case
	 * more memcaps get split after cap cache init below.
	 */
	bootres->ncaps += kcont->virtmem_used.ncaps +
			  kcont->virtmem_free.ncaps +
			  kcont->physmem_used.ncaps +
			  kcont->physmem_free.ncaps + 2;

	/* Initialise capability cache */
	kcont->cap_cache = init_resource_cache(bootres->ncaps,
					       sizeof(struct capability),
					       kcont, 0);

	/* Count boot pmds used so far and add them */
	bootres->npmds += pgd_count_pmds(&init_pgd);

	/*
	 * Calculate maximum possible pmds
	 * that may be used during this pmd
	 * cache init and add them.
	 */
	bootres->npmds += ((bootres->npmds * PMD_SIZE) / PMD_MAP_SIZE);
	if (!is_aligned(bootres->npmds * PMD_SIZE, PMD_MAP_SIZE))
		bootres->npmds++;

	/* Initialise PMD cache */
	kcont->pmd_cache = init_resource_cache(bootres->npmds,
					       PMD_SIZE, kcont, 1);

}

/*
 * Do all system accounting for a given capability info
 * structure that belongs to a container, such as
 * count its resource requirements, remove its portion
 * from global kernel resource capabilities etc.
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
	case CAP_RTYPE_MAPPOOL:
		/* Speficies how many pmds can be mapped */
		bootres->npmds += cap->size;
		break;
	case CAP_RTYPE_CAPPOOL:
		/* Specifies how many new caps can be created */
		bootres->ncaps += cap->size;
		break;

	case CAP_RTYPE_VIRTMEM:
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
		if ((ret = memcap_unmap(&kcont->physmem_free,
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
 * Initializes the kernel container by describing both virtual
 * and physical memory. Then traverses cap_info structures
 * to figure out resource requirements of containers.
 */
int setup_boot_resources(struct boot_resources *bootres,
			 struct kernel_container *kcont)
{
	struct cap_info *cap;

	init_kernel_container(kcont);

	/* Number of containers known at compile-time */
	bootres->nconts = CONFIG_TOTAL_CONTAINERS;

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

	return 0;
}


/*
 * FIXME: Add error handling
 *
 * Initializes all system resources and handling of those
 * resources. First descriptions are done by allocating from
 * boot memory, once memory caches are initialized, boot
 * memory allocations are migrated over to caches.
 */
int init_system_resources(struct kernel_container *kcont)
{
	/* FIXME: Count kernel resources */
	struct boot_resources bootres;

	memset(&bootres, 0, sizeof(bootres));

	setup_boot_resources(&bootres, kcont);

	init_resource_allocators(&bootres, kcont);

	/* Create system containers */
	setup_containers(&bootres, kcont);

	/* Create real capabilities */
	kcont_setup_capabilities(&bootres, kcont);

	return 0;
}


