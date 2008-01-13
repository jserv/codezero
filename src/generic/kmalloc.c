/*
 * Memory pool based kmalloc.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/list.h>
#include <l4/lib/memcache.h>
#include <l4/generic/pgalloc.h>
#include INC_GLUE(memory.h)

/* Supports this many different kmalloc sizes */
#define KMALLOC_POOLS_MAX	5

struct kmalloc_mempool {
	int total;
	struct list_head pool_head[KMALLOC_POOLS_MAX];
};
struct kmalloc_mempool km_pool;

void init_kmalloc()
{
	for (int i = 0; i < KMALLOC_POOLS_MAX; i++)
		INIT_LIST_HEAD(&km_pool.pool_head[i]);
}

/*
 * Allocates memory from mem_caches that it generates on-the-fly,
 * for up to KMALLOC_POOLS_MAX different sizes.
 */
void *kmalloc(int size)
{
	struct mem_cache *cache, *n;
	int right_sized_pool_idx = -1;
       	int index;

	/* Search all existing pools for this size, and if found, free bufs */
	for (int i = 0; i < km_pool.total; i++) {
		list_for_each_entry_safe(cache, n, &km_pool.pool_head[i], list) {
			if (cache->struct_size == size) {
				right_sized_pool_idx = i;
				if (cache->free)
					return mem_cache_alloc(cache);
				else
					continue;
			} else
				break;
		}
	}

	/*
	 * No such pool list already available at hand, and we don't have room
	 * for new pool lists.
	 */
	if ((right_sized_pool_idx < 0) &&
	    (km_pool.total == KMALLOC_POOLS_MAX - 1)) {
		printk("kmalloc: Too many types of pool sizes requested. "
		       "Giving up.\n");
		BUG();
	}

	if (right_sized_pool_idx >= 0)
		index = right_sized_pool_idx;
	else
		index = km_pool.total++;

	/* Only allow up to page size */
	BUG_ON(size >= PAGE_SIZE);
	BUG_ON(!(cache = mem_cache_init(alloc_page(), PAGE_SIZE,
					size, 0)));
	list_add(&cache->list, &km_pool.pool_head[index]);
	return mem_cache_alloc(cache);
}

/* FIXME:
 * Horrible complexity O(n^2) because we don't know which cache
 * we're freeing from!!! But its simple. ;-)
 */
int kfree(void *p)
{
	struct mem_cache *cache, *tmp;

	for (int i = 0; i < km_pool.total; i++)
		list_for_each_entry_safe(cache, tmp, &km_pool.pool_head[i], list)
			if (!mem_cache_free(cache, p)) {
				if (mem_cache_is_empty(cache)) {
					list_del(&cache->list);
					free_page(cache);
					/* Total remains the same. */
				}
				return 0;
			}
	return -1;
}

void *kzalloc(int size)
{
	void *p = kmalloc(size);
	memset(p, 0, size);
	return p;
}

