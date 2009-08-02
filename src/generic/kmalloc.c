/*
 * Memory pool based kmalloc.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/list.h>
#include <l4/lib/memcache.h>
#include <l4/generic/resource.h>
#include INC_GLUE(memory.h)

/* Supports this many different kmalloc sizes */
#define KMALLOC_POOLS_MAX	5

struct kmalloc_pool_head {
	struct link cache_list;
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
		link_init(&km_pool.pool_head[i].cache_list);
		km_pool.pool_head[i].occupied = 0;
		km_pool.pool_head[i].total_caches = 0;
		km_pool.pool_head[i].cache_size = 0;
	}
	mutex_init(&km_pool.kmalloc_mutex);
}

void *kmalloc(int size)
{
	return 0;
}

int kfree(void *p)
{
	return 0;
}

void *kzalloc(int size)
{
	return 0;
}

