/*
 * Containers defined for current build.
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/generic/container.h>
#include <l4/generic/resource.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/api/errno.h>
#include INC_GLUE(memory.h)
#include INC_SUBARCH(mm.h)
#include INC_ARCH(linker.h)

int container_init(struct container *c)
{
	/* Allocate new container id */
	c->cid = id_new(&kernel_resources.container_ids);

	/* Init data structures */
	link_init(&c->pager_list);
	init_address_space_list(&c->space_list);
	init_ktcb_list(&c->ktcb_list);
	init_mutex_queue_head(&c->mutex_queue_head);

	/* Init pager structs */
	for (int i = 0; i < CONFIG_MAX_PAGERS_USED; i++)
		cap_list_init(&c->pager[i].cap_list);


	return 0;
}

struct container *container_create(void)
{
	struct container *c = alloc_container();

	container_init(c);

	return c;
}

void kres_insert_container(struct container *c,
			    struct kernel_resources *kres)
{
	list_insert(&c->list, &kres->containers.list);
	kres->containers.ncont++;
}

/*
 * TODO:
 *
 * Create a purer address_space_create that takes
 * flags for extra ops such as copying kernel tables,
 * user tables of an existing pgd etc.
 */

/*
 * Inspects pager parameters defined in the container,
 * and sets up an execution environment for the pager.
 *
 * This involves setting up pager's ktcb, space, utcb,
 * all ids, registers, and mapping its (perhaps) first
 * few pages in order to make it runnable.
 *
 * The first pager initialization is a special-case
 * since it uses the current kernel pgd.
 */
int init_pager(struct pager *pager,
		     struct container *cont,
		     pgd_table_t *current_pgd)
{
	struct ktcb *task;
	struct address_space *space;
	int first = !!current_pgd;

	/*
	 * Initialize dummy current capability list pointer
	 * so that capability accounting can be done as normal
	 *
	 * FYI: We're still on bootstack instead of current's
	 * real stack. Hence this is a dummy.
	 */
	current->cap_list_ptr = &pager->cap_list;

	/* New ktcb allocation is needed */
	task = tcb_alloc_init();

	/* If first, manually allocate/initalize space */
	if (first) {
		if (!(space = alloc_space())) {
			return -ENOMEM;
		}
		/* Set up space id */
		space->spid = id_new(&kernel_resources.space_ids);

		/* Initialize space structure */
		link_init(&space->list);
		mutex_init(&space->lock);
		space->pgd = current_pgd;
		address_space_attach(task, space);
	} else {
		/* Otherwise allocate conventionally */
		task->space = address_space_create(0);
	}

	/* Initialize ktcb */
	task_init_registers(task, pager->start_address);

	/* Initialize container/pager relationships */
	pager->tcb = task;
	task->pager = pager;
	task->pagerid = task->tid;
	task->container = cont;
	task->cap_list_ptr = &pager->cap_list;

	printk("%s: Mapping %lu pages from 0x%lx to 0x%lx for %s\n",
	       __KERNELNAME__,
	       __pfn(page_align_up(pager->memsize)),
	       pager->start_lma, pager->start_vma, cont->name);

	/* Map the task's space */
	add_mapping_pgd(pager->start_lma, pager->start_vma,
			page_align_up(pager->memsize),
			MAP_USR_DEFAULT_FLAGS, TASK_PGD(task));

	/* Initialize task scheduler parameters */
	sched_init_task(task, TASK_PRIO_PAGER);

	/* Give it a kick-start tick and make runnable */
	task->ticks_left = 1;
	sched_resume_async(task);

	/* Container list that keeps all tasks */
	tcb_add(task);

	return 0;
}

/*
 * Initialize all containers with their initial set of tasks,
 * spaces, scheduler parameters such that they can be started.
 */
int container_init_pagers(struct kernel_resources *kres,
			  pgd_table_t *current_pgd)
{
	struct container *cont;
	struct pager *pager;

	list_foreach_struct(cont, &kres->containers.list, list) {
		for (int i = 0; i < cont->npagers; i++) {
			pager = &cont->pager[i];

			/* First pager initializes specially */
			if (i == 0)
				init_pager(pager, cont, current_pgd);
			else
				init_pager(pager, cont, 0);
		}
	}

	return 0;
}

