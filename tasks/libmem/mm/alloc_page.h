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
	/* Keep track of page area lists and allocated caches for page areas. */
	struct list_head page_area_list;
	/* Caches of page areas that refer to any kind of data */
	struct list_head dcache_list;
	/* Caches of page areas that refer to cache pages */
	struct list_head ccache_list;
	/* A spare cache to aid when both caches are full */
	struct mem_cache *spare;
};

/* Initialises the page allocator */
void init_page_allocator(unsigned long start, unsigned long end);

/* Page allocation functions */
void *alloc_page(int quantity);
void *zalloc_page(int quantity);
int free_page(void *paddr);

#endif /* __ALLOC_PAGE_H__ */
