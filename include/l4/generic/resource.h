#ifndef __RESOURCES_H__
#define __RESOURCES_H__

/* Number of containers defined at compile-time */
#define TOTAL_CONTAINERS		1

#include <l4/generic/capability.h>

struct boot_resources {
	int nconts;
	int ncaps;
	int nids;
	int nthreads;
	int nspaces;
	int npmds;
	int nmutex;

	/* Kernel resource usage */
	int nkpmds;
	int nkpgds;
	int nkmemcaps;
};


struct kernel_container {
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
	struct mem_cache *address_space_cache;
	struct mem_cache *mutex_cache;
	struct mem_cache *cap_cache;
	struct mem_cache *cont_cache;
};

extern struct kernel_container kernel_container;

int init_system_resources(struct kernel_container *kcont);

#endif /* __RESOURCES_H__ */
