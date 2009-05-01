/*
 * Management of task utcb regions and own utcb.
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */
#include <l4lib/arch/utcb.h>
#include <l4/macros.h>
#include INC_GLUE(memlayout.h)
#include <mmap.h>
#include <utcb.h>
#include <lib/malloc.h>

/*
 * UTCB management in Codezero
 */

/* Globally disjoint utcb virtual region pool */
static struct address_pool utcb_region_pool;

int utcb_pool_init()
{
	int err;

	/* Initialise the global shm virtual address pool */
	if ((err = address_pool_init(&utcb_region_pool,
				     UTCB_AREA_START, UTCB_AREA_END)) < 0) {
		printf("UTCB address pool initialisation failed.\n");
		return err;
	}
	return 0;
}

void *utcb_new_address(int npages)
{
	return address_new(&utcb_region_pool, npages);
}

int utcb_delete_address(void *utcb_address, int npages)
{
	return address_del(&utcb_region_pool, utcb_address, npages);
}

/* Return an empty utcb slot in this descriptor */
unsigned long utcb_slot(struct utcb_desc *desc)
{
	int slot;

	if ((slot = id_new(desc->slots)) < 0)
		return 0;
	else
		return desc->utcb_base + (unsigned long)slot * UTCB_SIZE;
}

unsigned long task_new_utcb_desc(struct tcb *task)
{
	struct utcb_desc *d;

	/* Allocate a new descriptor */
	if (!(d	= kzalloc(sizeof(*d))))
		return 0;

	INIT_LIST_HEAD(&d->list);

	/* We currently assume UTCB is smaller than PAGE_SIZE */
       BUG_ON(UTCB_SIZE > PAGE_SIZE);

       /* Initialise utcb slots */
       d->slots = id_pool_new_init(PAGE_SIZE / UTCB_SIZE);

       /* Obtain a new and unique utcb base */
       d->utcb_base = (unsigned long)utcb_new_address(1);

       /* Add descriptor to tcb's chain */
       list_add(&d->list, &task->utcb_head->list);

       /* Obtain and return first slot */
       return d->utcb_base + UTCB_SIZE * id_new(d->slots);
}

/*
 * Upon fork, the utcb descriptor list is replaced by a new one, since it is a new
 * address space. A new utcb is allocated and mmap'ed for the child task
 * running in the newly created address space.
 *
 * The original privately mmap'ed regions for thread-local utcbs remain
 * as copy-on-write on the new task, just like mmap'ed the stacks for cloned
 * threads in the parent address space.
 *
 * Upon clone, naturally the utcb descriptor chain and vm_areas remain to be
 * shared. A new utcb slot is allocated either by using an empty one in one of
 * the existing mmap'ed utcb regions, or by mmaping a new utcb region.
 */
int task_setup_utcb(struct tcb *task)
{
	struct utcb_desc *udesc;
	unsigned long slot;
	void *err;

	/* Setting this up twice is a bug */
	BUG_ON(task->utcb_address);

	/* Search for an empty utcb slot already allocated to this space */
	list_for_each_entry(udesc, &task->utcb_head->list, list)
		if ((slot = utcb_slot(udesc)))
			goto out;

	/* Allocate a new utcb memory region and return its base */
	slot = task_new_utcb_desc(task);
out:

	/* Map this region as private to current task */
	if (IS_ERR(err = do_mmap(0, 0, task, slot,
				 VMA_ANONYMOUS | VMA_PRIVATE | VMA_FIXED |
				 VM_READ | VM_WRITE, 1))) {
		printf("UTCB: mmapping failed with %d\n", err);
		return (int)err;
	}

	/* Assign task's utcb address */
	task->utcb_address = slot;

	return 0;
}

