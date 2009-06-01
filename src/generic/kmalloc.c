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

struct kmalloc_pool_head {
	struct list_head cache_list;
	int occupied;
	int total_caches;
	int cache_size;
};

struct kmalloc_mempool {
	int total;
	struct kmalloc_pool_head pool_head[KMALLOC_POOLS_MAX];
	struct mutex kmalloc_mutex;
};
struct kmalloc_mempool km_pool;

void init_kmalloc()
{
	for (int i = 0; i < KMALLOC_POOLS_MAX; i++) {
		INIT_LIST_HEAD(&km_pool.pool_head[i].cache_list);
		km_pool.pool_head[i].occupied = 0;
		km_pool.pool_head[i].total_caches = 0;
		km_pool.pool_head[i].cache_size = 0;
	}
	mutex_init(&km_pool.kmalloc_mutex);
}

/*
 * KMALLOC implementation:
 *
 * Allocates memory from mem_caches that it generates on-the-fly,
 * for up to KMALLOC_POOLS_MAX different sizes.
 */
void *kmalloc(int size)
{
	struct mem_cache *cache;
	int right_sized_pool_idx = -1;
       	int index;

	BUG_ON(!size); /* It is a kernel bug if size is 0 */

	for (int i = 0; i < km_pool.total; i++) {
		/* Check if this pool has right size */
		if (km_pool.pool_head[i].cache_size == size) {
			right_sized_pool_idx = i;
			/*
			 * Found the pool, now see if any
			 * cache has available slots
			 */
			list_for_each_entry(cache, &km_pool.pool_head[i].cache_list,
					    list) {
				if (cache->free)
					return mem_cache_alloc(cache);
				else
					break;
			}
		}
	}

	/*
	 * All pools are allocated and none has requested size
	 */
	if ((right_sized_pool_idx < 0) &&
	    (km_pool.total == KMALLOC_POOLS_MAX - 1)) {
		printk("kmalloc: Too many types of pool sizes requested. "
		       "Giving up.\n");
		BUG();
	}

	/* A pool exists with given size? (But no cache in it is free) */
	if (right_sized_pool_idx >= 0)
		index = right_sized_pool_idx;
	else /* No pool of this size, allocate new by incrementing total */
		index = km_pool.total++; 

	/* Only allow up to page size */
	BUG_ON(size >= PAGE_SIZE);
	BUG_ON(!(cache = mem_cache_init(alloc_page(), PAGE_SIZE,
					size, 0)));
	printk("%s: Created new cache for size %d\n", __FUNCTION__, size);
	list_add(&cache->list, &km_pool.pool_head[index].cache_list);
	km_pool.pool_head[index].occupied = 1;
	km_pool.pool_head[index].total_caches++;
	km_pool.pool_head[index].cache_size = size;
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
		list_for_each_entry_safe(cache, tmp,
					 &km_pool.pool_head[i].cache_list,
					 list) {
			if (!mem_cache_free(cache, p)) {
				if (mem_cache_is_empty(cache)) {
					km_pool.pool_head[i].total_caches--;
					list_del(&cache->list);
					free_page(cache);
					/*
					 * Total remains the same but slot
					 * may have no caches left.
					 */
				}
				return 0;
			}
		}
	return -1;
}

void *kzalloc(int size)
{
	void *p = kmalloc(size);
	memset(p, 0, size);
	return p;
}

