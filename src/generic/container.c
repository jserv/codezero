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
	c->cid = id_new(&kernel_container.container_ids);

	/* Init data structures */
	link_init(&c->pager_list);
	init_address_space_list(&c->space_list);
	init_ktcb_list(&c->ktcb_list);
	init_mutex_queue_head(&c->mutex_queue_head);

	/* Ini pager structs */
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

void kcont_insert_container(struct container *c,
			    struct kernel_container *kcont)
{
	list_insert(&c->list, &kcont->containers.list);
	kcont->containers.ncont++;
}

/*
 * This searches pager capabilities and if it has a virtual memory area
 * defined as a UTCB, uses the first entry as its utcb. If not, it is
 * not an error, perhaps another pager will map its utcb.
 */
void task_setup_utcb(struct ktcb *task, struct pager *pager)
{
	struct capability *cap;

	/* Find a virtual memory capability with UTCB map permissions */
	list_foreach_struct(cap, &pager->cap_list.caps, list) {
		if (((cap->type & CAP_RTYPE_MASK) ==
		     CAP_RTYPE_VIRTMEM) &&
		    (cap->access & CAP_MAP_UTCB)) {
			/* Use first address slot as pager's utcb */
			task->utcb_address = __pfn_to_addr(cap->start);
			return;
		}
	}
}

#if 0

/*
 * NOTE: This is not useful because FP stores references to
 * old stack so even if stacks are copied, unwinding is not
 * possible, which makes copying pointless. If there was no
 * FP, it may make sense, but this is not tested.
 *
 * Copy current stack contents to new one,
 * and jump to that stack by modifying sp and frame pointer.
 */
int switch_stacks(struct ktcb *task)
{
	volatile register unsigned int stack asm("sp");
	volatile register unsigned int frameptr asm("fp");
	volatile register unsigned int newstack;
	unsigned int stack_size = (unsigned int)_bootstack - stack;

	newstack = align((unsigned long)task + PAGE_SIZE - 1,
			 STACK_ALIGNMENT);

	/* Copy stack contents to new stack */
	memcpy((void *)(newstack - stack_size),
	       (void *)stack, stack_size);

	/*
	 * Switch to new stack, as new stack
	 * minus currently used stack size
	 */
	stack = newstack - stack_size;

	/*
	 * Frame ptr is new stack minus the original
	 * difference from start of boot stack to current fptr
	 */
	frameptr = newstack -
		   ((unsigned int)_bootstack - frameptr);

	/* We should be able to return safely */
	return 0;
}
#endif

/*
 * TODO:
 *
 * Create a purer address_space_create that takes
 * flags for extra ops such as copying kernel tables,
 * user tables of an existing pgd etc.
 */

/*
 * The first pager initialization is a special-case
 * since it uses the current kernel pgd.
 */
int init_first_pager(struct pager *pager,
		     struct container *cont,
		     pgd_table_t *current_pgd)
{
	struct ktcb *task;
	struct address_space *space;

	/*
	 * Initialize dummy current capability list pointer
	 * so that capability accounting can be done as normal
	 */
	current->cap_list_ptr = &pager->cap_list;

	/*
	 * Find capability from pager's list, since
	 * there is no ktcb, no standard path to check
	 * per-task capability list yet.
	 */
	/* Use it to allocate ktcb */
	task = tcb_alloc_init();

	/* Initialize ktcb */
	task_init_registers(task, pager->start_vma);
	task_setup_utcb(task, pager);

	/* Allocate space structure */
	if (!(space = alloc_space()))
		return -ENOMEM;

	/* Set up space id */
	space->spid = id_new(&kernel_container.space_ids);

	/* Initialize space structure */
	link_init(&space->list);
	mutex_init(&space->lock);
	space->pgd = current_pgd;

	/* Initialize container relationships */
	task->space = space;
	pager->tcb = task;
	task->pager = pager;
	task->container = cont;
	task->cap_list_ptr = &pager->cap_list;

	/* Map the task's space */
	add_mapping_pgd(pager->start_lma, pager->start_vma,
			page_align_up(pager->memsize),
			MAP_USR_DEFAULT_FLAGS, TASK_PGD(task));

	printk("%s: Mapping %lu pages from 0x%lx to 0x%lx for %s\n",
	       __KERNELNAME__,
	       __pfn(page_align_up(pager->memsize)),
	       pager->start_lma, pager->start_vma, cont->name);

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
 * Inspects pager parameters defined in the container,
 * and sets up an execution environment for the pager.
 *
 * This involves setting up pager's ktcb, space, utcb,
 * all ids, registers, and mapping its (perhaps) first
 * few pages in order to make it runnable.
 */
int init_pager(struct pager *pager, struct container *cont)
{
	struct ktcb *task;

	/*
	 * Initialize dummy current capability list pointer
	 * so that capability accounting can be done as normal
	 * FYI: We're still on bootstack instead of current's
	 * real stack. Hence this is a dummy.
	 */
	current->cap_list_ptr = &pager->cap_list;

	/* Use it to allocate ktcb */
	task = tcb_alloc_init();

	task_init_registers(task, pager->start_vma);

	task_setup_utcb(task, pager);

	task->space = address_space_create(0);

	/* Initialize container relationships */
	pager->tcb = task;
	task->pager = pager;
	task->container = cont;
	task->cap_list_ptr = &pager->cap_list;

	add_mapping_pgd(pager->start_lma, pager->start_vma,
			page_align_up(pager->memsize),
			MAP_USR_DEFAULT_FLAGS, TASK_PGD(task));

	printk("%s: Mapping %lu pages from 0x%lx to 0x%lx for %s\n",
	       __KERNELNAME__, __pfn(page_align_up(pager->memsize)),
	       pager->start_lma, pager->start_vma, cont->name);

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
int container_init_pagers(struct kernel_container *kcont,
			  pgd_table_t *current_pgd)
{
	struct container *cont;
	struct pager *pager;
 	int pgidx = 0;

	list_foreach_struct(cont, &kcont->containers.list, list) {
		for (int i = 0; i < cont->npagers; i++) {
			pager = &cont->pager[i];

			/* First pager initializes specially */
			if (pgidx == 0)
				init_first_pager(pager, cont,
						 current_pgd);
			else
				init_pager(pager, cont);
			pgidx++;
		}
	}

	return 0;
}

