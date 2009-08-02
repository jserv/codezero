/*
 * Global physical memory descriptions.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/physmem.h>
#include <l4/generic/resource.h>
#include <l4/generic/tcb.h>
#include <l4/lib/list.h>
#include <l4/lib/spinlock.h>
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memlayout.h)
#include INC_GLUE(memory.h)
#include INC_PLAT(offsets.h)
#include INC_PLAT(printascii.h)
#include INC_ARCH(linker.h)

struct page_bitmap page_map;

static void init_page_map(unsigned long start, unsigned long end)
{
	page_map.pfn_start = __pfn(start);
	page_map.pfn_end = __pfn(end);
	set_page_map(start, __pfn(end - start), 0);
}

/*
 * Marks pages in the global page_map as used or unused.
 *
 * @start = start page address to set, inclusive.
 * @numpages = number of pages to set.
 */
int set_page_map(unsigned long start, int numpages, int val)
{
	unsigned long pfn_start = __pfn(start);
	unsigned long pfn_end = __pfn(start) + numpages;
	unsigned long pfn_err = 0;

	if (page_map.pfn_start > pfn_start || page_map.pfn_end < pfn_start) {
		pfn_err = pfn_start;
		goto error;
	}
	if (page_map.pfn_end < pfn_end || page_map.pfn_start > pfn_end) {
		pfn_err = pfn_end;
		goto error;
	}

	if (val)
		for (int i = pfn_start; i < pfn_end; i++)
			page_map.map[BITWISE_GETWORD(i)] |= BITWISE_GETBIT(i);
	else
		for (int i = pfn_start; i < pfn_end; i++)
			page_map.map[BITWISE_GETWORD(i)] &= ~BITWISE_GETBIT(i);
	return 0;
error:
	BUG_MSG("Given page area is out of system page_map range: 0x%lx\n",
		pfn_err << PAGE_BITS);
	return -1;
}

/* Describes physical memory boundaries of the system. */
struct memdesc physmem;

/* Fills in the physmem structure with free physical memory information */
void physmem_init()
{
	unsigned long start = (unsigned long)_start_kernel;
	unsigned long end = (unsigned long)_end_kernel;

	/* Initialise page map */
	init_page_map(PHYS_MEM_START, PHYS_MEM_END);

	/* Mark kernel areas as used */
	set_page_map(virt_to_phys(start), __pfn(end - start), 1);

	/* Map initial pgd area as used */
	start = (unsigned long)__pt_start;
	end = (unsigned long)__pt_end;
	set_page_map(virt_to_phys(TASK_PGD(current)), __pfn(end - start), 1);

	physmem.start = PHYS_MEM_START;
	physmem.end = PHYS_MEM_END;

	physmem.free_cur = __svc_images_end;
	physmem.free_end = PHYS_MEM_END;
	physmem.numpages = (PHYS_MEM_START - PHYS_MEM_END) / PAGE_SIZE;
}

void memory_init()
{
	//init_pgalloc();
}

