/*
 * Global physical memory descriptions.
 *
 * Copyright (C) 2007 - 2009 Bahadir Balban
 */
#include <l4/macros.h>
#include <l4/config.h>
#include <l4/types.h>
#include <l4/lib/list.h>
#include <l4/lib/math.h>
#include <l4/api/thread.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include INC_GLUE(memory.h)

#include <l4lib/arch/syslib.h>
#include <stdio.h>
#include <init.h>
#include <physmem.h>

struct page_bitmap page_map;	/* Bitmap of used/unused pages at bootup */
struct memdesc physmem;		/* Initial, primitive memory descriptor */
struct membank membank[1];	/* The memory bank */
struct page *page_array;	/* The physical page array based on mem bank */


static void init_page_map(unsigned long pfn_start, unsigned long pfn_end)
{
	page_map.pfn_start = pfn_start;
	page_map.pfn_end = pfn_end;
	set_page_map(&page_map, pfn_start, pfn_end - pfn_start, 0);
}

/*
 * Marks pages in the global page_map as used or unused.
 *
 * @start = start page address to set, inclusive.
 * @numpages = number of pages to set.
 */
int set_page_map(struct page_bitmap *page_map, unsigned long pfn_start,
		 int numpages, int val)
{
	unsigned long pfn_end = pfn_start + numpages;
	unsigned long pfn_err = 0;

	if (page_map->pfn_start > pfn_start || page_map->pfn_end < pfn_start) {
		pfn_err = pfn_start;
		goto error;
	}
	if (page_map->pfn_end < pfn_end || page_map->pfn_start > pfn_end) {
		pfn_err = pfn_end;
		goto error;
	}

	if (val)
		for (int i = pfn_start; i < pfn_end; i++)
			page_map->map[BITWISE_GETWORD(i)] |= BITWISE_GETBIT(i);
	else
		for (int i = pfn_start; i < pfn_end; i++)
			page_map->map[BITWISE_GETWORD(i)] &= ~BITWISE_GETBIT(i);
	return 0;
error:
	BUG_MSG("Given page area is out of system page_map range: 0x%lx\n",
		pfn_err << PAGE_BITS);
	return -1;
}

/* Allocates page descriptors and initialises them using page_map information */
void init_physmem_secondary(struct initdata *initdata, struct membank *membank)
{
	struct page_bitmap *pmap = initdata->page_map;
	int npages = pmap->pfn_end - pmap->pfn_start;

	/* Allocation marks for the struct page array; npages, start, end */
	int pg_npages, pg_spfn, pg_epfn;
	unsigned long ffree_addr;

	/*
	 * Means the page array won't map one to one to pfns. That's ok,
	 * but we dont allow it for now.
	 */
	// BUG_ON(pmap->pfn_start);

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
	pg_npages = __pfn(page_align_up((sizeof(struct page) * npages)));

	/* These are relative pfn offsets to the start of the memory bank */

	/* FIXME:
	 * 1.) These values were only right when membank started from pfn 0.
	 * 2.) Use set_page_map to set page map below instead of manually.
	 */
	pg_spfn = __pfn(membank[0].free);
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
		link_init(&membank[0].page_array[i].list);

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

	/* Test that page/phys macros work */
	BUG_ON(phys_to_page(page_to_phys(&page_array[5])) != &page_array[5])
}


/* Fills in the physmem structure with free physical memory information */
void init_physmem_primary(struct initdata *initdata)
{
	unsigned long pfn_start, pfn_end, pfn_images_end = 0;
	struct bootdesc *bootdesc = initdata->bootdesc;

	/* Initialise page map from physmem capability */
	init_page_map(initdata->physmem->start,
		      initdata->physmem->end);

	/* Set initdata pointer to initialized page map */
	initdata->page_map = &page_map;

	/* Mark pager and other boot task areas as used */
	for (int i = 0; i < bootdesc->total_images; i++) {
		pfn_start = __pfn(page_align_up(bootdesc->images[i].phys_start));
		pfn_end = __pfn(page_align_up(bootdesc->images[i].phys_end));
		if (pfn_end > pfn_images_end)
			pfn_images_end = pfn_end;
		set_page_map(&page_map, pfn_start, pfn_end - pfn_start, 1);
	}

	physmem.start = initdata->physmem->start;
	physmem.end = initdata->physmem->end;

	physmem.free_cur = pfn_images_end;
	physmem.free_end = physmem.end;
	physmem.numpages = __pfn(physmem.end - physmem.start);
}

