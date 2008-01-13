/*
 * A proof-of-concept linked-list based page allocator.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <l4/config.h>
#include <l4/macros.h>
#include <l4/types.h>
#include <l4/lib/list.h>
#include <l4lib/arch/syscalls.h>
#include "alloc_page.h"
#include INC_GLUE(memory.h)
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memlayout.h)

struct page_allocator allocator;

static struct mem_cache *new_dcache();

/*
 * Allocate a new page area from @area_sources_start. If no areas left,
 * allocate a new cache first, allocate page area from the new cache.
 */
static struct page_area *new_page_area(struct page_allocator *p,
				       struct list_head *ccache)
{
	struct mem_cache *cache;
	struct page_area *new_area;
	struct list_head *cache_list;

	if (ccache)
		cache_list = ccache;
	else
		cache_list = &p->dcache_list;

	list_for_each_entry(cache, cache_list, list)
		if ((new_area = mem_cache_alloc(cache)) != 0) {
			new_area->cache = cache;
			return new_area;
		}

	/* Must not reach here if a ccache is already used. */
	BUG_ON(ccache);

	if ((cache = new_dcache(p)) == 0)
		return 0; /* Denotes out of memory */

	new_area = mem_cache_alloc(cache);
	new_area->cache = cache;
	return new_area;
}

/* Given the page @quantity, finds a free region, divides and returns new area. */
static struct page_area *
get_free_page_area(int quantity, struct page_allocator *p,
		   struct list_head *cache_list)
{
	struct page_area *new, *area;

	if (quantity <= 0)
		return 0;

	list_for_each_entry(area, &p->page_area_list, list) {
		/* Free but needs dividing */
		if (area->numpages > quantity && !area->used) {
			area->numpages -= quantity;
			if (!(new = new_page_area(p, cache_list)))
				return 0; /* No more pages */
			new->pfn = area->pfn + area->numpages;
			new->numpages = quantity;
			new->used = 1;
			INIT_LIST_HEAD(&new->list);
			list_add(&new->list, &area->list);
			return new;
		/* Free and exact size match, no need to divide. */
		} else if (area->numpages == quantity && !area->used) {
			area->used = 1;
			return area;
		}
	}
	/* No more pages */
	return 0;
}

void *alloc_page(int quantity)
{
	struct page_area *p = get_free_page_area(quantity, &allocator, 0);

	if (p)
		return (void *)__pfn_to_addr(p->pfn);
	else
		return 0;
}

/*
 * Helper to allocate a page using an internal page area cache. Returns
 * a virtual address because these allocations are always internally referenced.
 */
void *alloc_page_using_cache(struct page_allocator *a, struct list_head *c)
{
	struct page_area *p = get_free_page_area(1, a, c);

	if (p)
		return l4_map_helper((void *)__pfn_to_addr(p->pfn), 1);
	else
		return 0;

}

/*
 * There's still free memory, but allocator ran out of page areas stored in
 * dcaches. In this case, the ccache supplies a new page area, which is used to
 * describe a page that stores a new dcache. If ccache is also out of page areas
 * it adds the spare cache to ccache_list, uses that for the current allocation,
 * and allocates a new spare cache for future use
 */
static struct mem_cache *new_dcache(struct page_allocator *p)
{
	void *dpage;	/* Page that keeps data cache */
	void *spage;	/* Page that keeps spare cache */

	if((dpage = alloc_page_using_cache(p, &p->ccache_list)))
		return mem_cache_init(dpage, PAGE_SIZE,
				      sizeof(struct page_area), 0);

	/* If ccache is also full, add the spare page_area cache to ccache */
	list_add(&p->spare->list, &p->ccache_list);

	/* This must not fail now, and satisfy at least two page requests. */
	BUG_ON(mem_cache_total_empty(p->spare) < 2);
	BUG_ON(!(dpage = alloc_page_using_cache(p, &p->ccache_list)));
	BUG_ON(!(spage = alloc_page_using_cache(p, &p->ccache_list)));

	/* Initialise the new spare and return the new dcache. */
	p->spare = mem_cache_init(spage, PAGE_SIZE, sizeof(struct page_area),0);
	return mem_cache_init(dpage, PAGE_SIZE, sizeof(struct page_area), 0);
}

/*
 * Only to be used at initialisation. Allocates memory caches that contain
 * page_area elements by incrementing the free physical memory mark by
 * PAGE_SIZE.
 */
static struct mem_cache *new_allocator_startup_cache(unsigned long *start)
{
	struct page_area *area;
	struct mem_cache *cache;

	cache = mem_cache_init(l4_map_helper((void *)*start, 1), PAGE_SIZE,
			       sizeof(struct page_area), 0);
	area = mem_cache_alloc(cache);

	/* Initialising the dummy just for illustration */
	area->pfn = __pfn(*start);
	area->numpages = 1;
	area->cache = cache;
	INIT_LIST_HEAD(&area->list);

	/* FIXME: Should I add this to the page area list? */
	*start += PAGE_SIZE;
	return cache;
}

/*
 * All physical memory is tracked by a simple linked list implementation. A
 * single list contains both used and free page_area descriptors. Each page_area
 * describes a continuous region of physical pages, indicating its location by
 * it's pfn.
 *
 * alloc_page() keeps track of all page-granuled memory, except the bits that
 * were in use before the allocator initialised. This covers anything that is
 * outside the @start @end range. This includes the page tables, first caches
 * allocated by this function, compile-time allocated kernel data and text.
 * Also other memory regions like IO are not tracked by alloc_page() but by
 * other means.
 */
void init_page_allocator(unsigned long start, unsigned long end)
{
	struct page_area *freemem;
	struct mem_cache *dcache, *ccache;

	INIT_LIST_HEAD(&allocator.dcache_list);
	INIT_LIST_HEAD(&allocator.ccache_list);
	INIT_LIST_HEAD(&allocator.page_area_list);

	/* Primary cache list that stores page areas of regular data. */
	dcache = new_allocator_startup_cache(&start);
	list_add(&dcache->list, &allocator.dcache_list);

	/* The secondary cache list that stores page areas of caches */
	ccache = new_allocator_startup_cache(&start);
	list_add(&ccache->list, &allocator.ccache_list);

	/* Initialise first area that describes all of free physical memory */
	freemem = mem_cache_alloc(dcache);
	INIT_LIST_HEAD(&freemem->list);
	freemem->pfn = __pfn(start);
	freemem->numpages = __pfn(end) - freemem->pfn;
	freemem->cache = dcache;
	freemem->used = 0;

	/* Add it as the first unused page area */
	list_add(&freemem->list, &allocator.page_area_list);

	/* Allocate and add the spare page area cache */
	allocator.spare = mem_cache_init(l4_map_helper(alloc_page(1), 1),
					 PAGE_SIZE, sizeof(struct page_area),
					 0);
}

/* Merges two page areas, frees area cache if empty, returns the merged area. */
struct page_area *merge_free_areas(struct page_area *before,
				   struct page_area *after)
{
	struct mem_cache *c;

	BUG_ON(before->pfn + before->numpages != after->pfn);
	BUG_ON(before->used || after->used)
	BUG_ON(before == after);

	before->numpages += after->numpages;
	list_del(&after->list);
	c = after->cache;
	mem_cache_free(c, after);

	/* Recursively free the cache page */
	if (mem_cache_is_empty(c))
		BUG_ON(free_page(l4_unmap_helper(c, 1)) < 0)
	return before;
}

static int find_and_free_page_area(void *addr, struct page_allocator *p)
{
	struct page_area *area, *prev, *next;

	/* First find the page area to be freed. */
	list_for_each_entry(area, &p->dcache_list, list)
		if (__pfn_to_addr(area->pfn) == (unsigned long)addr &&
		    area->used) {	/* Found it */
			area->used = 0;
			goto found;
		}
	return -1; /* Finished the loop, but area not found. */

found:
	/* Now merge with adjacent areas, if possible */
	if (area->list.prev != &p->dcache_list) {
		prev = list_entry(area->list.prev, struct page_area, list);
		if (!prev->used)
			area = merge_free_areas(prev, area);
	}
	if (area->list.next != &p->dcache_list) {
		next = list_entry(area->list.next, struct page_area, list);
		if (!next->used)
			area = merge_free_areas(area, next);
	}
	return 0;
}

int free_page(void *paddr)
{
	return find_and_free_page_area(paddr, &allocator);
}

