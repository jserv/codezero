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
#include <l4/generic/tcb.h>
#include <l4/lib/idpool.h>
#include <l4/api/mutex.h>
#include <l4/lib/list.h>
#include <l4/lib/idpool.h>

/* Container macro. No locks needed! */
#define this_container		(current->container)

struct container {
	/* Unique container id */
	l4id_t cid;

	/* List of address spaces */
	struct address_space_list space_list;

	/* List of threads */
	struct ktcb_list ktcb_list;

	/* ID pools for threads and spaces */
	struct id_pool *thread_id_pool;
	struct id_pool *space_id_pool;

	/* Scheduling structs */
	struct scheduler scheduler;

	/* Mutex list for all userspace mutexes */
	struct mutex_queue_head mutex_queue_head;

	/*
	 * Capabilities that apply to this container
	 *
	 * Threads, address spaces, mutex queues, cpu share ...
	 * Pagers possess these capabilities.
	 */
	struct capability caps[5] /* threadpool, spacepool, mutexpool, cpupool, mempool */
};

/* The array of containers present on the system */
extern struct container container[];

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
	struct cap_info caps[];
};

/*
 * This auto-generated structure is
 * used to create run-time containers
 */
struct container_info {
	char *name;
	int npagers;
	struct pager_info pagers[];
};

extern struct container_info cinfo[];

#endif /* __CONTAINER_H__ */

