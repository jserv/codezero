/*
 * Simple kernel memory allocator built on top of memcache
 * implementation.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/memcache.h>
#include <l4/lib/list.h>
#include <l4/generic/space.h>
#include <l4/generic/kmalloc.h>
#include <l4/generic/pgalloc.h>
#include <l4/generic/physmem.h>
#include INC_GLUE(memory.h)

#define	PGALLOC_PGD_CACHE		0
#define PGALLOC_PMD_CACHE		1
#define PGALLOC_PG_CACHE		2
#define PGALLOC_CACHE_TOTAL		3

/* The initial chunk of physical memory allocated before any pagers. */
#define PGALLOC_INIT_GRANT		SZ_1MB

/* Covers 3 main types of memory needed by the kernel. */
struct pgalloc {
	struct list_head cache_list[3];
};
static struct pgalloc pgalloc;

void pgalloc_add_new_cache(struct mem_cache *cache, int cidx)
{
	INIT_LIST_HEAD(&cache->list);
	BUG_ON(cidx >= PGALLOC_CACHE_TOTAL || cidx < 0);
	list_add(&cache->list, &pgalloc.cache_list[cidx]);
}

void calc_kmem_usage_per_grant(kmem_usage_per_grant_t *params)
{
	/* Pmds, pgds, pages in numbers, per grant */
	int pmds_per_task_avg = params->task_size_avg / PMD_MAP_SIZE;
	int pmds_per_kmem_grant = params->tasks_per_kmem_grant * pmds_per_task_avg;
	int pgds_per_kmem_grant = params->tasks_per_kmem_grant * 1;
	int pgs_per_kmem_grant =  params->tasks_per_kmem_grant * 1;

	/* Now everything in Kbs */
	params->pmd_total = pmds_per_kmem_grant * PMD_SIZE;
	params->pgd_total = pgds_per_kmem_grant * PGD_SIZE;
	params->pg_total = pgs_per_kmem_grant * PAGE_SIZE;
	params->extra = params->grant_size -
	       		(params->pgd_total + params->pmd_total +
			 params->pg_total);
}

int pgalloc_add_new_grant(unsigned long pfn, int npages)
{
	unsigned long physical = __pfn_to_addr(pfn);
	void *virtual = (void *)phys_to_virt(physical);
	struct mem_cache *pgd_cache, *pmd_cache, *pg_cache;
	kmem_usage_per_grant_t params;

	/* First map the whole grant */
	add_mapping(physical, phys_to_virt(physical), __pfn_to_addr(npages),
		    MAP_SVC_RW_FLAGS);

	/* Calculate how to divide buffer into different caches */
	params.task_size_avg = TASK_AVERAGE_SIZE;
	params.grant_size = npages * PAGE_SIZE;

	/* Calculate pools for how many tasks from this much grant */
	params.tasks_per_kmem_grant = (__pfn(SZ_1MB) * TASKS_PER_1MB_GRANT) /
				      __pfn(params.grant_size);
	calc_kmem_usage_per_grant(&params);

	/* Create the caches, least alignment-needing, most, then others. */
	pmd_cache = mem_cache_init(virtual, params.pmd_total, PMD_SIZE, 1);
	virtual += params.pmd_total;
	pgd_cache = mem_cache_init(virtual, params.pgd_total, PGD_SIZE, 1);
	virtual += params.pgd_total;
	pg_cache = mem_cache_init(virtual, params.pg_total + params.extra,
				  PAGE_SIZE, 1);

	/* Add the caches */
	pgalloc_add_new_cache(pgd_cache, PGALLOC_PGD_CACHE);
	pgalloc_add_new_cache(pmd_cache, PGALLOC_PMD_CACHE);
	pgalloc_add_new_cache(pg_cache, PGALLOC_PG_CACHE);

	return 0;
}

void init_pgalloc(void)
{
	int initial_grant = PGALLOC_INIT_GRANT;

	for (int i = 0; i < PGALLOC_CACHE_TOTAL; i++)
		INIT_LIST_HEAD(&pgalloc.cache_list[i]);

	/* Grant ourselves with an initial chunk of physical memory */
	physmem.free_cur = page_align_up(physmem.free_cur);
	set_page_map(physmem.free_cur, __pfn(initial_grant), 1);
	pgalloc_add_new_grant(__pfn(physmem.free_cur), __pfn(initial_grant));
	physmem.free_cur += initial_grant;

	/* Activate kmalloc */
	init_kmalloc();
}

void pgalloc_remove_cache(struct mem_cache *cache)
{
	list_del_init(&cache->list);
}

static inline void *pgalloc_from_cache(int cidx)
{
	struct mem_cache *cache, *n;

	list_for_each_entry_safe(cache, n, &pgalloc.cache_list[cidx], list)
		if (mem_cache_total_empty(cache))
			return mem_cache_zalloc(cache);
	return 0;
}

int kfree_to_cache(int cidx, void *virtual)
{
	struct mem_cache *cache, *n;

	list_for_each_entry_safe(cache, n, &pgalloc.cache_list[cidx], list)
		if (mem_cache_free(cache, virtual) == 0)
			return 0;
	return -1;
}

void *alloc_page(void)
{
	return pgalloc_from_cache(PGALLOC_PG_CACHE);
}

void *alloc_pmd(void)
{
	pmd_table_t *pmd;

	if (!(pmd = alloc_boot_pmd()))
		pmd = pgalloc_from_cache(PGALLOC_PMD_CACHE);

	return pmd;
}

void *alloc_pgd(void)
{
	return pgalloc_from_cache(PGALLOC_PGD_CACHE);
}

int free_page(void *v)
{
	return kfree_to_cache(PGALLOC_PG_CACHE, v);
}

int free_pmd(void *v)
{
	return kfree_to_cache(PGALLOC_PMD_CACHE, v);
}

int free_pgd(void *v)
{
	return kfree_to_cache(PGALLOC_PGD_CACHE, v);
}

void *zalloc_page(void)
{
	void *p = alloc_page();
	memset(p, 0, PAGE_SIZE);
	return p;
}

