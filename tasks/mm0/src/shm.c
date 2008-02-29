/*
 * Copyright (C) 2007 Bahadir Balban
 *
 * Posix shared memory implementation
 */
#include <shm.h>
#include <stdio.h>
#include <task.h>
#include <mmap.h>
#include <l4/lib/string.h>
#include <kmalloc/kmalloc.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <lib/idpool.h>
#include <lib/addr.h>
#include <lib/spinlock.h>
#include <l4/api/errno.h>
#include <l4/lib/list.h>
#include <l4/macros.h>
#include <l4/config.h>
#include <l4/types.h>
#include INC_GLUE(memlayout.h)
#include <posix/sys/ipc.h>
#include <posix/sys/shm.h>
#include <posix/sys/types.h>

/* The list of shared memory areas that are already set up and working */
static struct list_head shm_desc_list;

/* The single global in-memory swap file for shared memory segments */

/* Unique shared memory ids */
static struct id_pool *shm_ids;

/* Globally disjoint shm virtual address pool */
static struct address_pool shm_vaddr_pool;

void shm_init()
{
	INIT_LIST_HEAD(&shm_desc_list);

	/* Initialise shm id pool */
	shm_ids = id_pool_new_init(SHM_AREA_MAX);

	/* Initialise the global shm virtual address pool */
	address_pool_init(&shm_vaddr_pool, SHM_AREA_START, SHM_AREA_END);
}

/*
 * TODO:
 * Implement means to return back ipc results, i.e. sender always does ipc_sendrecv()
 * and it blocks on its own receive queue. Server then responds back without blocking.
 *
 * Later on: mmap can be done using vm_areas and phsyical pages can be accessed by vm_areas.
 */
static int do_shmat(struct shm_descriptor *shm, void *shm_addr, int shmflg,
		    l4id_t tid)
{
	struct tcb *task = find_task(tid);
	int err;

	if (!task) {
		printf("%s:%s: Cannot find caller task with tid %d\n",
		       __TASKNAME__, __FUNCTION__, tid);
		BUG();
	}

	/*
	 * Currently shared memory base addresses are the same among all
	 * processes for every unique shm segment. They line up easier on
	 * the shm swap file this way. Also currently shm_addr argument is
	 * ignored, and mm0 allocates shm segment addresses.
	 */
	if (shm->shm_addr)
		shm_addr = shm->shm_addr;
	else
		shm_addr = address_new(&shm_vaddr_pool, __pfn(shm->size));

	BUG_ON(!is_page_aligned(shm_addr));

	/*
	 * mmap the area to the process as shared. Page fault handler would
	 * handle allocating and paging-in the shared pages.
	 *
	 * For anon && shared pages do_mmap() handles allocation of the
	 * shm swap file and the file offset for the segment. The segment can
	 * be identified because segment virtual address is globally unique
	 * per segment and its the same for all the system tasks.
	 */
	if ((err = do_mmap(0, 0, task, (unsigned long)shm_addr,
			   VM_READ | VM_WRITE | VMA_ANON | VMA_SHARED,
			   shm->size)) < 0) {
		printf("do_mmap: Mapping shm area failed with %d.\n", err);
		BUG();
	} else
		printf("%s: %s: Success.\n", __TASKNAME__, __FUNCTION__);

	/* Now update the shared memory descriptor */
	shm->refcnt++;

	return 0;
}

void *sys_shmat(l4id_t requester, l4id_t shmid, void *shmaddr, int shmflg)
{
	struct shm_descriptor *shm_desc, *n;
	int err;

	list_for_each_entry_safe(shm_desc, n, &shm_desc_list, list) {
		if (shm_desc->shmid == shmid) {
			if ((err = do_shmat(shm_desc, shmaddr,
					    shmflg, requester) < 0)) {
				l4_ipc_return(err);
				return 0;
			} else
				break;
		}
	}
	l4_ipc_return(0);

	return 0;
}

int do_shmdt(struct shm_descriptor *shm, l4id_t tid)
{
	struct tcb *task = find_task(tid);
	int err;

	if (!task) {
		printf("%s:%s: Internal error. Cannot find task with tid %d\n",
		       __TASKNAME__, __FUNCTION__, tid);
		BUG();
	}
	if ((err = do_munmap(shm->shm_addr, shm->size, task)) < 0) {
		printf("do_munmap: Unmapping shm segment failed with %d.\n",
		       err);
		BUG();
	}
	return err;
}

int sys_shmdt(l4id_t requester, const void *shmaddr)
{
	struct shm_descriptor *shm_desc, *n;
	int err;

	list_for_each_entry_safe(shm_desc, n, &shm_desc_list, list) {
		if (shm_desc->shm_addr == shmaddr) {
			if ((err = do_shmdt(shm_desc, requester) < 0)) {
				l4_ipc_return(err);
				return 0;
			} else
				break;
		}
	}
	l4_ipc_return(0);

	return 0;
}

static struct shm_descriptor *shm_new(key_t key)
{
	/* It doesn't exist, so create a new one */
	struct shm_descriptor *shm_desc;

	if ((shm_desc = kzalloc(sizeof(struct shm_descriptor))) < 0)
		return 0;
	if ((shm_desc->shmid = id_new(shm_ids)) < 0)
		return 0;

	shm_desc->key = (int)key;
	INIT_LIST_HEAD(&shm_desc->list);
	list_add(&shm_desc->list, &shm_desc_list);
	return shm_desc;
}

int sys_shmget(key_t key, int size, int shmflg)
{
	struct shm_descriptor *shm_desc;

	/* First check argument validity */
	if (size > SHM_SHMMAX || size < SHM_SHMMIN) {
		l4_ipc_return(-EINVAL);
		return 0;
	}

	/*
	 * IPC_PRIVATE means create a no-key shm area, i.e. private to this
	 * process so that it would only share it with its descendants.
	 */
	if (key == IPC_PRIVATE) {
		key = -1;		/* Our meaning of no key */
		if (!shm_new(key))
			l4_ipc_return(-ENOSPC);
		else
			l4_ipc_return(0);
		return 0;
	}

	list_for_each_entry(shm_desc, &shm_desc_list, list) {
		if (shm_desc->key == key) {
			/*
			 * Exclusive means create request
			 * on existing key should fail.
			 */
			if ((shmflg & IPC_CREAT) && (shmflg & IPC_EXCL))
				l4_ipc_return(-EEXIST);
			else
				/* Found it but do we have a size problem? */
				if (shm_desc->size < size)
					l4_ipc_return(-EINVAL);
				else /* Return shmid of the existing key */
					l4_ipc_return(shm_desc->shmid);
			return 0;
		}
	}
	/* Key doesn't exist and create set, so we create */
	if (shmflg & IPC_CREAT)
		if (!(shm_desc = shm_new(key)))
			l4_ipc_return(-ENOSPC);
		else
			l4_ipc_return(shm_desc->shmid);
	else	/* Key doesn't exist, yet create isn't set, its an -ENOENT */
		l4_ipc_return(-ENOENT);

	return 0;
}


