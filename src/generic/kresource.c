/*
 * Initialize system resource management.
 *
 * Copyright (C) 2009 Bahadir Balban
 */

/*
 * Here are the steps used to initialize system resources:
 *
 * Check total physical memory
 * Check container memory capabilities
 * Find biggest unused physical memory region
 * Calculate how much memory is used by all containers
 * Initialize a slab-like allocator for all resources.
 * Copy boot allocations to real allocations accounted to containers and kernel.
 *    E.g. initial page table may become page table of a container pager.
 *    First few pmds used belong to kernel usage, etc.
 * Delete all boot memory and add it to physical memory pool.
 */

#define MEM_FLAGS_VIRTUAL		(1 << 0)
#define MEM_AREA_CACHED			(1 << 1)

struct mem_area {
	struct link list;
	l4id_t mid;
	unsigned long start;
	unsigned long end;
	unsigned long npages;
	unsigned long flags;
};


void init_system_resources()
{
	struct mem_area *physmem = alloc_bootmem(sizeof(physmem), 4);
	struct mem_area *kernel_used = alloc_bootmem(sizeof(physmem), 4);

	/* Initialize the first memory descriptor for total physical memory */
	physmem.start = PHYS_MEM_START;
	physmem.end = PHYS_MEM_END;
	physmem.mid = 0;
	physmem.npages = (physmem.end - physmem.start) >> PAGE_BITS;

	/* Figure out current kernel usage */
	kernel_used.start = virt_to_phys(_kernel_start);
	kernel_used.end = virt_to_phys(_kernel_end);

	/* Figure out each container's physical memory usage */
	for (int i = 0; i < containers->total; i++) {
	}
}




