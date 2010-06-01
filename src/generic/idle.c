/*
 * Idle task initialization and maintenance
 *
 * Copyright (C) 2010 B Labs Ltd.
 * Author: Bahadir Balban
 */

#include <l4/generic/cap-types.h>
#include <l4/api/capability.h>
#include <l4/generic/capability.h>
#include <l4/generic/smp.h>
#include <l4/generic/tcb.h>
#include <l4/generic/bootmem.h>
#include <l4/generic/container.h>
#include INC_GLUE(mapping.h)

/*
 * Set up current stack's beginning, and initial page tables
 * as a valid task environment for idle task for current cpu
 *
 * Note, container pointer is initialized later when
 * containers are in shape.
 */
void setup_idle_task()
{
	memset(current, 0, sizeof(struct ktcb));

	current->space = &kernel_resources.init_space;
	TASK_PGD(current) = &init_pgd;

	/* Initialize space caps list */
	cap_list_init(&current->space->cap_list);

	/* Init scheduler structs */
	sched_init_task(current, TASK_PRIO_NORMAL);

	/* Set up never-to-be used fields as invalid for precaution */
	current->pager = 0;
	current->tgid = -1;

	/*
	 * Set pointer to global kernel page tables.
	 * This is required early for early-stage pgd mappings
	 */
#if defined(CONFIG_SUBARCH_V7)
	kernel_resources.pgd_global = &init_global_pgd;
#endif

}

void secondary_idle_task_init(void)
{
	/* This also has its spid allocated by primary */
	current->space = &kernel_resources.init_space;
	TASK_PGD(current) = &init_pgd;

	/* Need to assign a thread id */
	current->tid = id_new(&kernel_resources.ktcb_ids);

	/* Set affinity */
	current->affinity = smp_get_cpuid();

	/* Set up never-to-be used fields as invalid for precaution */
	current->pager = 0;
	current->tgid = -1;

	/* Init scheduler structs */
	sched_init_task(current, TASK_PRIO_NORMAL);

	sched_resume_async(current);
}

