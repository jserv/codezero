/*
 * Copyright (C) 2008 Bahadir Balban
 */
#ifndef __SHM_H__
#define __SHM_H__

#include <l4/lib/list.h>
#include <l4/api/space.h>
#include <l4/macros.h>
#include <l4lib/types.h>
#include <task.h>
#include <posix/sys/ipc.h>
#include <posix/sys/shm.h>
#include <posix/sys/types.h>

struct shm_descriptor {
	int key;
	l4id_t shmid;
	void *shm_addr;
	unsigned long npages;
	struct vm_file *devzero;
};

#if 0
struct shm_descriptor {
	int key;			/* IPC key supplied by user task */
	l4id_t shmid;			/* SHM area id, allocated by mm0 */
	struct list_head list;		/* SHM list, used by mm0 */
	struct vm_file *owner;
	void *shm_addr;			/* The virtual address for segment. */
	unsigned long size;		/* Size of the area in pages */
	unsigned int flags;
	int refcnt;
};
#endif

#define SHM_AREA_MAX			64	/* Up to 64 shm areas */

/* Up to 10 pages per area, and at least 1 byte (implies 1 page) */
#define SHM_SHMMIN			1
#define SHM_SHMMAX			10

/* Initialises shared memory bookkeeping structures */
void shm_init();

void *shmat_shmget_internal(struct tcb *task, key_t key, void *shmaddr);
struct vm_file *shm_new(key_t key, unsigned long npages);

#endif /* __SHM_H__ */
