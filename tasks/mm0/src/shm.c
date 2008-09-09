/*
 * Copyright (C) 2007, 2008 Bahadir Balban
 *
 * Posix shared memory implementation
 */
#include <shm.h>
#include <stdio.h>
#include <task.h>
#include <mmap.h>
#include <utcb.h>
#include <vm_area.h>
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

#define shm_file_to_desc(shm_file)	\
	((struct shm_descriptor *)shm_file->priv_data)

/* The list of shared memory areas that are already set up and working */
static LIST_HEAD(shm_file_list);

/* Unique shared memory ids */
static struct id_pool *shm_ids;

/* Globally disjoint shm virtual address pool */
static struct address_pool shm_vaddr_pool;

void shm_init()
{
	/* Initialise shm id pool */
	shm_ids = id_pool_new_init(SHM_AREA_MAX);

	/* Initialise the global shm virtual address pool */
	address_pool_init(&shm_vaddr_pool, SHM_AREA_START, SHM_AREA_END);
}

/*
 * Attaches to given shm segment mapped at shm_addr if the shm descriptor
 * does not already have a base address assigned. If neither shm_addr nor
 * the descriptor has an address, allocates one from the shm address pool.
 */
static void *do_shmat(struct vm_file *shm_file, void *shm_addr, int shmflg,
		      struct tcb *task)
{
	struct shm_descriptor *shm = shm_file_to_desc(shm_file);
	unsigned int vmflags;
	int err;

	if (!task) {
		printf("%s:%s: Cannot find caller task with tid %d\n",
		       __TASKNAME__, __FUNCTION__, task->tid);
		BUG();
	}

	if (shm_addr)
		shm_addr = (void *)page_align(shm_addr);

	/* Determine mmap flags for segment */
	if (shmflg & SHM_RDONLY)
		vmflags = VM_READ | VMA_SHARED | VMA_ANONYMOUS;
	else
		vmflags = VM_READ | VM_WRITE |
			  VMA_SHARED | VMA_ANONYMOUS;

	/*
	 * The first user of the segment who supplies a valid
	 * address sets the base address of the segment. Currently
	 * all tasks use the same address for each unique segment.
	 */

	/* First user? */
	if (!shm_file->vm_obj.nlinks)
		if (mmap_address_validate(task, (unsigned long)shm_addr,
					  vmflags))
			shm->shm_addr = shm_addr;
		else
			shm->shm_addr = address_new(&shm_vaddr_pool,
						    shm->npages);
	else /* Address must be already assigned */
		BUG_ON(!shm->shm_addr);

	/*
	 * mmap the area to the process as shared. Page fault handler would
	 * handle allocating and paging-in the shared pages.
	 */
	if ((err = do_mmap(shm_file, 0, task, (unsigned long)shm->shm_addr,
			   vmflags, shm->npages)) < 0) {
		printf("do_mmap: Mapping shm area failed with %d.\n", err);
		BUG();
	}

	return shm->shm_addr;
}

int sys_shmat(l4id_t requester, l4id_t shmid, void *shmaddr, int shmflg)
{
	struct vm_file *shm_file, *n;
	struct tcb *task = find_task(requester);

	list_for_each_entry_safe(shm_file, n, &shm_file_list, list) {
		if (shm_file_to_desc(shm_file)->shmid == shmid) {
			shmaddr = do_shmat(shm_file, shmaddr,
					   shmflg, task);

			/*
			 * UTCBs get special treatment here. If the task
			 * is attaching to its utcb, mm0 prefaults it so
			 * that it can access it later on whether or not
			 * the task makes a syscall to mm0 without first
			 * faulting the utcb.
			 */
			/*
			if ((unsigned long)shmaddr == task->utcb_address)
				utcb_prefault(task, VM_READ | VM_WRITE);
			*/

			l4_ipc_return((int)shmaddr);
			return 0;
		}
	}

	l4_ipc_return(-EINVAL);
	return 0;
}

int do_shmdt(struct vm_file *shm, l4id_t tid)
{
	struct tcb *task = find_task(tid);
	int err;

	if (!task) {
		printf("%s:%s: Internal error. Cannot find task with tid %d\n",
		       __TASKNAME__, __FUNCTION__, tid);
		BUG();
	}
	if ((err = do_munmap(shm_file_to_desc(shm)->shm_addr,
			     shm_file_to_desc(shm)->npages, task)) < 0) {
		printf("do_munmap: Unmapping shm segment failed with %d.\n",
		       err);
		BUG();
	}

	return err;
}

int sys_shmdt(l4id_t requester, const void *shmaddr)
{
	struct vm_file *shm_file, *n;
	int err;

	list_for_each_entry_safe(shm_file, n, &shm_file_list, list) {
		if (shm_file_to_desc(shm_file)->shm_addr == shmaddr) {
			if ((err = do_shmdt(shm_file, requester) < 0)) {
				l4_ipc_return(err);
				return 0;
			} else
				break;
		}
	}

	l4_ipc_return(-EINVAL);
	return 0;
}


/* Creates an shm area and glues its details with shm pager and devzero */
struct vm_file *shm_new(key_t key, unsigned long npages)
{
	struct shm_descriptor *shm_desc;
	struct vm_file *shm_file;

	BUG_ON(!npages);

	/* Allocate file and shm structures */
	if (IS_ERR(shm_file = vm_file_create()))
		return PTR_ERR(shm_file);

	if (!(shm_desc = kzalloc(sizeof(struct shm_descriptor)))) {
		kfree(shm_file);
		return PTR_ERR(-ENOMEM);
	}

	/* Initialise the shm descriptor */
	if (IS_ERR(shm_desc->shmid = id_new(shm_ids))) {
		kfree(shm_file);
		kfree(shm_desc);
		return PTR_ERR(shm_desc->shmid);
	}
	shm_desc->key = (int)key;
	shm_desc->npages = npages;

	/* Initialise the file */
	shm_file->length = __pfn_to_addr(npages);
	shm_file->type = VM_FILE_SHM;
	shm_file->priv_data = shm_desc;

	/* Initialise the vm object */
	shm_file->vm_obj.pager = &swap_pager;
	shm_file->vm_obj.flags = VM_OBJ_FILE | VM_WRITE;

	list_add(&shm_file->list, &shm_file_list);
	list_add(&shm_file->vm_obj.list, &vm_object_list);

	return shm_file;
}

/*
 * Fast internal path to do shmget/shmat() together for mm0's
 * convenience. Works for existing areas.
 */
void *shmat_shmget_internal(struct tcb *task, key_t key, void *shmaddr)
{
	struct vm_file *shm_file;
	struct shm_descriptor *shm_desc;

	list_for_each_entry(shm_file, &shm_file_list, list) {
		shm_desc = shm_file_to_desc(shm_file);
		/* Found the key, shmat that area */
		if (shm_desc->key == key)
			return do_shmat(shm_file, shmaddr,
					0, task);
	}

	return PTR_ERR(-EEXIST);
}

/*
 * FIXME: Make sure hostile tasks don't subvert other tasks' utcbs
 * by early-registring their utcb address here.
 */
int sys_shmget(key_t key, int size, int shmflg)
{
	unsigned long npages = __pfn(page_align_up(size));
	struct vm_file *shm;

	/* First check argument validity */
	if (npages > SHM_SHMMAX || npages < SHM_SHMMIN) {
		l4_ipc_return(-EINVAL);
		return 0;
	}

	/*
	 * IPC_PRIVATE means create a no-key shm area, i.e. private to this
	 * process so that it would only share it with its forked children.
	 */
	if (key == IPC_PRIVATE) {
		key = -1;		/* Our meaning of no key */
		if (!(shm = shm_new(key, npages)))
			l4_ipc_return(-ENOSPC);
		else
			l4_ipc_return(shm_file_to_desc(shm)->shmid);
		return 0;
	}

	list_for_each_entry(shm, &shm_file_list, list) {
		struct shm_descriptor *shm_desc = shm_file_to_desc(shm);

		if (shm_desc->key == key) {
			/*
			 * Exclusive means a create request
			 * on an existing key should fail.
			 */
			if ((shmflg & IPC_CREAT) && (shmflg & IPC_EXCL))
				l4_ipc_return(-EEXIST);
			else
				/* Found it but do we have a size problem? */
				if (shm_desc->npages < npages)
					l4_ipc_return(-EINVAL);
				else /* Return shmid of the existing key */
					l4_ipc_return(shm_desc->shmid);
			return 0;
		}
	}

	/* Key doesn't exist and create is set, so we create */
	if (shmflg & IPC_CREAT)
		if (!(shm = shm_new(key, npages)))
			l4_ipc_return(-ENOSPC);
		else
			l4_ipc_return(shm_file_to_desc(shm)->shmid);
	else	/* Key doesn't exist, yet create isn't set, its an -ENOENT */
		l4_ipc_return(-ENOENT);

	return 0;
}

