/*
 * Some ktcb related data
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/preempt.h>
#include <l4/lib/idpool.h>
#include <l4/api/kip.h>
#include INC_ARCH(exception.h)
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memory.h)

/* ID pools for threads and spaces. */
struct id_pool *thread_id_pool;
struct id_pool *space_id_pool;
struct id_pool *tgroup_id_pool;

/* Hash table for all existing tasks */
struct list_head global_task_list;

/* Offsets for ktcb fields that are accessed from assembler */
unsigned int need_resched_offset = offsetof(struct ktcb, ts_need_resched);
unsigned int syscall_regs_offset = offsetof(struct ktcb, syscall_regs);

/*
 * Every thread has a unique utcb region that is mapped to its address
 * space as its context is loaded. The utcb region is a function of
 * this mapping and its offset that is reached via the KIP UTCB pointer
 */
void task_update_utcb(struct ktcb *cur, struct ktcb *next)
{
	/* Update the KIP pointer */
	kip.utcb = next->utcb_address;

	/* We stick with KIP update and no private tls mapping for now */
#if 0
	/*
	 * Unless current and next are in the same address
	 * space and sharing the same physical utcb page, we
	 * update the mapping
	 */
	if (cur->utcb_phys != next->utcb_phys)
		add_mapping(page_align(next->utcb_phys),
			    page_align(next->utcb_virt),
			    page_align_up(UTCB_SIZE),
			    MAP_USR_RW_FLAGS);
	/*
	 * If same physical utcb but different pgd, it means two
	 * address spaces share the same utcb. We treat this as a
	 * bug for now.
	 */
	else
		BUG_ON(cur->pgd != next->pgd);
#endif

}

