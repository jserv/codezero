#ifndef __SHM_H__
#define __SHM_H__

#include <l4/api/space.h>
#include <l4/lib/list.h>
#include <l4/macros.h>
#include <l4lib/types.h>

struct shm_descriptor {
	int key;			/* IPC key supplied by user task */
	l4id_t shmid;			/* SHM area id, allocated by mm0 */
	struct list_head list;		/* SHM list, used by mm0 */
	struct vm_file *owner;
	void *shm_addr;			/* The virtual address for segment. */
	unsigned long size;		/* Size of the area */
	unsigned int flags;
	int refcnt;
};

#define SHM_AREA_MAX			64	/* Up to 64 shm areas */

/* Up to 10 pages per area, and at least 1 byte (implies 1 page) */
#define SHM_SHMMIN			1
#define SHM_SHMMAX			(PAGE_SIZE * 10)

/*
 * NOTE: This flags the unique shm vaddr pool. If its not globally unique
 * and shm areas are cached, on ARMv5 cache aliasing occurs.
 */
#define SHM_DISJOINT_VADDR_POOL

/* Initialises shared memory bookkeeping structures */
void shm_init();

#endif /* __SHM_H__ */
