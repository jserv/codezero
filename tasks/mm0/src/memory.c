/*
 * Initialise the memory structures.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <init.h>
#include <memory.h>
#include <l4/macros.h>
#include <l4/config.h>
#include <l4/types.h>
#include <l4/generic/space.h>
#include <l4lib/arch/syslib.h>
#include INC_GLUE(memory.h)
#include INC_SUBARCH(mm.h)
#include <memory.h>

struct membank membank[1];
struct page *page_array;

void *phys_to_virt(void *addr)
{
	return addr + INITTASK_OFFSET;
}

void *virt_to_phys(void *addr)
{
	return addr - INITTASK_OFFSET;
}

/* Allocates page descriptors and initialises them using page_map information */
void init_physmem(struct initdata *initdata, struct membank *membank)
{
	struct page_bitmap *pmap = &initdata->page_map;
	int npages = pmap->pfn_end - pmap->pfn_start;

	/* Allocation marks for the struct page array */
	int pg_npages, pg_spfn, pg_epfn;
	unsigned long ffree_addr;

	/*
	 * Means the page array won't map one to one to pfns. That's ok,
	 * but we dont allow it for now.
	 */
	BUG_ON(pmap->pfn_start);

	membank[0].start = __pfn_to_addr(pmap->pfn_start);
	membank[0].end = __pfn_to_addr(pmap->pfn_end);

	/* First find the first free page after last used page */
	for (int i = 0; i < npages; i++)
		if ((pmap->map[BITWISE_GETWORD(i)] & BITWISE_GETBIT(i)))
			membank[0].free = (i + 1) * PAGE_SIZE;
	BUG_ON(membank[0].free >= membank[0].end);

	/*
	 * One struct page for every physical page. Calculate how many pages
	 * needed for page structs, start and end pfn marks.
	 */
	pg_npages = __pfn((sizeof(struct page) * npages));

	/* These are relative pfn offsets to the start of the memory bank */
	pg_spfn = __pfn(membank[0].free) - __pfn(membank[0].start);
	pg_epfn = pg_spfn + pg_npages;

	/* Use free pages from the bank as the space for struct page array */
	membank[0].page_array = l4_map_helper((void *)membank[0].free,
					      pg_npages);
	/* Update free memory left */
	membank[0].free += pg_npages * PAGE_SIZE;

	/* Update page bitmap for the pages used for the page array */
	for (int i = pg_spfn; i < pg_epfn; i++)
		pmap->map[BITWISE_GETWORD(i)] |= BITWISE_GETBIT(i);

	/* Initialise the page array */
	for (int i = 0; i < npages; i++) {
		INIT_LIST_HEAD(&membank[0].page_array[i].list);

		/* Set use counts for pages the kernel has already used up */
		if (!(pmap->map[BITWISE_GETWORD(i)] & BITWISE_GETBIT(i)))
			membank[0].page_array[i].refcnt = -1;
		else	/* Last page used +1 is free */
			ffree_addr = (i + 1) * PAGE_SIZE;
	}

	/* First free address must come up the same for both */
	BUG_ON(ffree_addr != membank[0].free);

	/* Set global page array to this bank's array */
	page_array = membank[0].page_array;
}

