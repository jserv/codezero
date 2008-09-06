#ifndef __ALLOC_PAGE_H__
#define __ALLOC_PAGE_H__

#include <memcache/memcache.h>

/* List member to keep track of free and unused physical pages.
 * Has PAGE_SIZE granularity */
struct page_area {
	struct list_head list;
	unsigned int used;	/* Used or free */
	unsigned int pfn;	/* Base pfn */
	unsigned int numpages;	/* Number of pages this region covers */
	struct mem_cache *cache;/* The cache used when freeing the page area for
				 * quickly finding where the area is stored. */
};

struct page_allocator {
	struct list_head page_area_list;
	struct list_head pga_cache_list;
	int pga_free;
};

/* Initialises the page allocator */
void init_page_allocator(unsigned long start, unsigned long end);

/* Page allocation functions */
void *alloc_page(int quantity);
void *zalloc_page(int quantity);
int free_page(void *paddr);

#endif /* __ALLOC_PAGE_H__ */
