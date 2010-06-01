/*
 * Description of resources on the system
 *
 * Copyright (C) 2009 Bahadir Balban
 */

#ifndef __RESOURCES_H__
#define __RESOURCES_H__

/* Number of containers defined at compile-time */
#include <l4/generic/capability.h>
#include <l4/generic/space.h>
#include <l4/lib/list.h>
#include <l4/lib/mutex.h>
#include <l4/lib/idpool.h>
#include INC_SUBARCH(mm.h)

struct boot_resources {
	int nconts;
	int ncaps;
	int nthreads;
	int nspaces;
	int npmds;
	int nmutex;
};

/* List of containers */
struct container_head {
	int ncont;
	struct link list;
	struct spinlock lock;
};

static inline void
container_head_init(struct container_head *chead)
{
	chead->ncont = 0;
	link_init(&chead->list);
	spin_lock_init(&chead->lock);
}

/* Hash table for all existing tasks */
struct ktcb_list {
	struct link list;
	struct spinlock list_lock;
	int count;
};

/*
 * Everything on the platform is described and stored
 * in the structure below.
 */
struct kernel_resources {
	l4id_t cid;

	/* System id pools */
	struct id_pool space_ids;
	struct id_pool ktcb_ids;
	struct id_pool resource_ids;
	struct id_pool container_ids;
	struct id_pool mutex_ids;
	struct id_pool capability_ids;

	/* List of all containers */
	struct container_head containers;

	/* Physical memory caps, used/unused */
	struct cap_list physmem_used;
	struct cap_list physmem_free;

	/* Virtual memory caps, used/unused */
	struct cap_list virtmem_used;
	struct cap_list virtmem_free;

	/* Device memory caps, used/unused */
	struct cap_list devmem_used;
	struct cap_list devmem_free;

	struct mem_cache *pgd_cache;
	struct mem_cache *pmd_cache;
	struct mem_cache *ktcb_cache;
	struct mem_cache *space_cache;
	struct mem_cache *mutex_cache;
	struct mem_cache *cap_cache;
	struct mem_cache *cont_cache;

	/* Zombie thread list */
	DECLARE_PERCPU(struct ktcb_list, zombie_list);

#if defined(CONFIG_SUBARCH_V7)
	/* Global page tables on split page tables */
	pgd_global_table_t *pgd_global;
#endif
	struct address_space init_space;
};

extern struct kernel_resources kernel_resources;

void pgd_free(void *addr);
void pmd_cap_free(void *addr, struct cap_list *clist);
void space_cap_free(void *addr, struct cap_list *clist);
void ktcb_cap_free(void *addr, struct cap_list *clist);
void mutex_cap_free(void *addr);

pgd_table_t *pgd_alloc(void);
pmd_table_t *pmd_cap_alloc(struct cap_list *clist);
struct address_space *space_cap_alloc(struct cap_list *clist);
struct ktcb *ktcb_cap_alloc(struct cap_list *clist);
struct mutex_queue *mutex_cap_alloc(void);

int init_system_resources(struct kernel_resources *kres);

void setup_idle_caps(struct kernel_resources *kres);

#endif /* __RESOURCES_H__ */
