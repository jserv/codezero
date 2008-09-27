/*
 * Utcb address allocation for user tasks.
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
#include <vm_area.h>
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

void *utcb_vaddr_new(void)
{
	return address_new(&utcb_vaddr_pool, 1);
}

int utcb_vaddr_del(void *utcb_addr)
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

#if 0

To be ditched
/*
 * Triggered during a sys_shmat() by a client task when mapping its utcb.
 * This prefaults the utcb and maps it in to mm0 so that it can freely
 * access it anytime later.
 */
int utcb_prefault(struct tcb *task, unsigned int vmflags)
{
	int err;
	struct page *pg;

	/* First map in the page to task with given flags, e.g. read/write */
	if ((err = prefault_page(task, task->utcb_address, vmflags)) < 0) {
		printf("%s: Failed: %d\n", __FUNCTION__, err);
		return err;
	}

	/*
	 * Get the topmost page. Since we did both a VM_READ and VM_WRITE
	 * prefault, this gets a writeable instead of a read-only page.
	 */
	pg = task_virt_to_page(task, task->utcb_address);
	if (!pg || IS_ERR(pg)) {
		printf("%s: Cannot retrieve task %d's utcb page.\n",
		       __FUNCTION__, task->tid);
		BUG();
	}

	/* Map it in to self */
	l4_map((void *)page_to_phys(pg), (void *)task->utcb_address, 1,
	       MAP_USR_RW_FLAGS, self_tid());

	/* Flag that says this task's utcb is mapped to mm0 as r/w */
	task->utcb_mapped = 1;

	return 0;
}

#endif
