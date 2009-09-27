/*
 * Definitions for Codezero Containers
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <l4/generic/scheduler.h>
#include <l4/generic/space.h>
#include <l4/generic/capability.h>
#include <l4/generic/resource.h>
#include <l4/generic/tcb.h>
#include <l4/lib/idpool.h>
#include <l4/api/mutex.h>
#include <l4/lib/list.h>
#include <l4/lib/idpool.h>

#define curcont			(current->container)

#define CONFIG_CONTAINER_NAMESIZE		64
#define CONFIG_MAX_CAPS_USED			14
#define CONFIG_MAX_PAGERS_USED			2

/* Container macro. No locks needed! */

struct pager {
	struct ktcb *tcb;
	unsigned long start_lma;
	unsigned long start_vma;
	unsigned long start_address;
	unsigned long stack_address;
	unsigned long memsize;
	struct cap_list cap_list;
};


struct container {
	l4id_t cid;				/* Unique container id */
	int npagers;				/* # of pagers */
	struct link list;			/* List ref for containers */
	struct address_space_list space_list;	/* List of address spaces */
	char name[CONFIG_CONTAINER_NAMESIZE];	/* Name of container */
	struct ktcb_list ktcb_list;		/* List of threads */
	struct link pager_list;			/* List of pagers */

	struct id_pool *thread_id_pool;		/* Id pools for thread/spaces */
	struct id_pool *space_id_pool;

	struct mutex_queue_head mutex_queue_head; /* Userspace mutex list */

	/*
	 * Capabilities that apply to this container
	 *
	 * Threads, address spaces, mutex queues, cpu share ...
	 * Pagers possess these capabilities.
	 */
	/* threadpool, spacepool, mutexpool, cpupool, mempool */
	struct pager pager[CONFIG_MAX_PAGERS_USED];
};

/* Compact, raw capability structure */
struct cap_info {
	unsigned int type;
	u32 access;
	unsigned long start;
	unsigned long end;
	unsigned long size;
};


struct pager_info {
	unsigned long pager_lma;
	unsigned long pager_vma;
	unsigned long pager_size;
	unsigned long start_address;
	unsigned long stack_address;

	/* Number of capabilities defined */
	int ncaps;

	/*
	 * Zero or more ipc caps,
	 * One or more thread pool caps,
	 * One or more space pool caps,
	 * One or more exregs caps,
	 * One or more tcontrol caps,
	 * One or more cputime caps,
	 * One or more physmem caps,
	 * One or more virtmem caps,
	 * Zero or more umutex caps,
	 */
	struct cap_info caps[CONFIG_MAX_CAPS_USED];
};

/*
 * This auto-generated structure is
 * used to create run-time containers
 */
struct container_info {
	char name[CONFIG_CONTAINER_NAMESIZE];
	int npagers;
	struct pager_info pager[CONFIG_MAX_PAGERS_USED];
};

extern struct container_info cinfo[];

void kcont_insert_container(struct container *c,
			    struct kernel_container *kcont);

struct container *container_create(void);

int container_init_pagers(struct kernel_container *kcont,
			  pgd_table_t *current_pgd);

int init_containers(struct kernel_container *kcont);

#endif /* __CONTAINER_H__ */

