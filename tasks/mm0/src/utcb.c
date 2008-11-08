/*
 * utcb address allocation for user tasks.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <stdio.h>
#include <utcb.h>
#include <lib/addr.h>
#include <l4/macros.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <task.h>
#include <shm.h>
#include <vm_area.h>
#include <syscalls.h>
#include INC_GLUE(memlayout.h)

static struct address_pool utcb_vaddr_pool;

int utcb_pool_init()
{
	int err;
	if ((err = address_pool_init(&utcb_vaddr_pool,
				     UTCB_AREA_START,
				     UTCB_AREA_END)) < 0) {
		printf("UTCB address pool initialisation failed: %d\n", err);
		return err;
	}
	return 0;
}

void *utcb_new_address(void)
{
	return address_new(&utcb_vaddr_pool, 1);
}

int utcb_delete_address(void *utcb_addr)
{
	return address_del(&utcb_vaddr_pool, utcb_addr, 1);
}

/*
 * Sends utcb address information to requester task, allocates
 * an address if it doesn't exist and the requester is asking
 * for its own. The requester then uses this address as a shm key and
 * maps its own utcb via shmget/shmat.
 */
void *task_send_utcb_address(struct tcb *sender, l4id_t taskid)
{
	struct tcb *task = find_task(taskid);

	/* Is the task asking for its own utcb address */
	if (sender->tid == taskid) {
		/* It hasn't got one allocated. */
		BUG_ON(!task->utcb);

		/* Return it to requester */
		return task->utcb;

	/* A task is asking for someone else's utcb */
	} else {
		/* Only vfs is allowed to do so yet, because its a server */
		if (sender->tid == VFS_TID) {
			/*
			 * Return utcb address to requester. Note if there's
			 * none allocated so far, requester gets 0. We don't
			 * allocate one here.
			 */
			return task->utcb;
		}
	}
	return 0;
}

int utcb_map_to_task(struct tcb *owner, struct tcb *mapper, unsigned int flags)
{
	struct vm_file *utcb_shm;

	/* Allocate a new utcb address */
	if (flags & UTCB_NEW_ADDRESS)
		owner->utcb = utcb_new_address();
	else if (!owner->utcb)
		BUG();

	/* Create a new shared memory segment for utcb */
	if (flags & UTCB_NEW_SHM)
		if (IS_ERR(utcb_shm = shm_new((key_t)owner->utcb,
					      __pfn(DEFAULT_UTCB_SIZE))))
		return (int)utcb_shm;

	/* Map the utcb to mapper */
	if (IS_ERR(shmat_shmget_internal(mapper, (key_t)owner->utcb,
					 owner->utcb)))
		BUG();

	/* Prefault the owner's utcb to mapper's address space */
	if (flags & UTCB_PREFAULT)
		for (int i = 0; i < __pfn(DEFAULT_UTCB_SIZE); i++)
			prefault_page(mapper, (unsigned long)owner->utcb +
				      __pfn_to_addr(i), VM_READ | VM_WRITE);
	return 0;
}

int utcb_unmap_from_task(struct tcb *owner, struct tcb *mapper)
{
	return sys_shmdt(mapper, owner->utcb);
}
