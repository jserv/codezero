/*
 * Containers defined for current build.
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/generic/container.h>
#include <l4/generic/resource.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/generic/bootmem.h>
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
	cap_list_init(&c->cap_list);

	/* Init pager structs */
	c->pager = alloc_bootmem(sizeof(c->pager[0]) *
				 CONFIG_MAX_PAGERS_USED, 0);
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
	spin_lock(&kres->containers.lock);
	list_insert(&c->list, &kres->containers.list);
	kres->containers.ncont++;
	spin_unlock(&kres->containers.lock);
}

struct container *container_find(struct kernel_resources *kres, l4id_t cid)
{
	struct container *c;

	spin_lock(&kres->containers.lock);
	list_foreach_struct(c, &kres->containers.list, list) {
		if (c->cid == cid) {
			spin_unlock(&kres->containers.lock);
			return c;
		}
	}
	spin_unlock(&kres->containers.lock);
	return 0;
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
	 * Set up dummy current cap_list so that cap accounting
	 * can be done to this pager. Note, that we're still on
	 * bootstack.
	 */
	cap_list_move(&current->cap_list, &pager->cap_list);

	/* Setup dummy container pointer so that curcont works */
	current->container = cont;

	/* New ktcb allocation is needed */
	task = tcb_alloc_init(cont->cid);

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
		cap_list_init(&space->cap_list);
		space->pgd = current_pgd;
		address_space_attach(task, space);
	} else {
		/* Otherwise allocate conventionally */
		space = address_space_create(0);
		address_space_attach(task, space);
	}

	/* Initialize ktcb */
	task_init_registers(task, pager->start_address);

	/* Initialize container/pager relationships */
	task->pagerid = task->tid;
	task->tgid = task->tid;
	task->container = cont;

	/* Add the address space to container space list */
	address_space_add(task->space);

#if 0
	/* Initialize uninitialized capability fields while on dummy */
	list_foreach_struct(cap, &current->cap_list.caps, list) {
		/* Initialize owner */
		cap->owner = task->tid;

		/*
		 * Initialize resource id, if possible.
		 * Currently this is a temporary hack where
		 * we allow only container ids and _assume_
		 * that container is pager's own container.
		 *
		 * See capability resource ids for info
		 */
		if (cap_rtype(cap) == CAP_RTYPE_CONTAINER)
			cap->resid = task->container->cid;
		else
			cap->resid = CAP_RESID_NONE;
	}
#endif

	printk("%s: Mapping 0x%lx bytes (%lu pages) from 0x%lx to 0x%lx for %s\n",
	       __KERNELNAME__, pager->memsize,
	       __pfn(page_align_up(pager->memsize)),
	       pager->start_lma, pager->start_vma, cont->name);

	/* Map the task's space */
	add_mapping_pgd(pager->start_lma, pager->start_vma,
			page_align_up(pager->memsize),
			MAP_USR_DEFAULT_FLAGS, TASK_PGD(task));

	/* Move capability list from dummy to task's space cap list */
	cap_list_move(&task->space->cap_list, &current->cap_list);

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
 * All first-level dynamically allocated resources
 * are initialized, which includes the pager thread ids
 * and pager space ids.
 *
 * Update all capability target ids that target such
 * run-time allocated resources.
 */
int update_dynamic_capids(struct kernel_resources *kres)
{
	struct ktcb *pager, *tpager;
	struct container *cont, *tcont;
	struct capability *cap;

	/* Containers */
	list_foreach_struct(cont, &kres->containers.list, list) {
		/* Pagers */
		list_foreach_struct(pager, &cont->ktcb_list.list, task_list) {
			/* Capabilities */
			list_foreach_struct(cap,
					    &pager->space->cap_list.caps,
					    list) {

				/* They all shall be owned by their pager */
				cap->owner = pager->tid;

				/*
				 * Pager Space/Thread targets need updating
				 * from the given static container id to their
				 * run-time allocated ids.
				 */

				/* Quantity caps don't have target ids */
				if (cap_type(cap) == CAP_TYPE_QUANTITY)
					cap->resid = CAP_RESID_NONE;

				/*
				 * Space _always_ denotes current pager's
				 * space. Other containers are not addressable
				 * by space ids.
				 */
				if (cap_rtype(cap) == CAP_RTYPE_SPACE)
					cap->resid = pager->space->spid;

				/*
				 * Thread _always_denotes another container's
				 * pager. There is simply no other reasonable
				 * thread target in the system.
				 */
				if (cap_rtype(cap) == CAP_RTYPE_THREAD) {

					/* Find target container */
					if (!(tcont =
					      container_find(kres,
						             cap->resid))) {
						printk("FATAL: Capability "
						       "configured to target "
						       "non-existent "
						       "container.\n");
						BUG();

					}

					/* Find its pager */
					if (list_empty(&tcont->ktcb_list.list)) {
						printk("FATAL: Pager"
						       "does not exist in "
						       "container %d.\n",
						       tcont->cid);
						BUG();
					}

					tpager =
					link_to_struct(
						tcont->ktcb_list.list.next,
						struct ktcb, task_list);

					/* Assign pager's thread id to cap */
					cap->resid = tpager->tid;
				}
			}
		}
	}

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
	int first = 1;

	list_foreach_struct(cont, &kres->containers.list, list) {
		for (int i = 0; i < cont->npagers; i++) {
			pager = &cont->pager[i];

			/* First pager initializes specially */
			if (first) {
				init_pager(pager, cont, current_pgd);
				first = 0;
			} else {
				init_pager(pager, cont, 0);
			}
		}
	}

	/* Update any capability fields that were dynamically allocated */
	update_dynamic_capids(kres);

	return 0;
}




