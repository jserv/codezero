/*
 * Some ktcb related data
 *
 * Copyright (C) 2007 - 2009 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/container.h>
#include <l4/generic/preempt.h>
#include <l4/generic/space.h>
#include <l4/lib/idpool.h>
#include <l4/api/ipc.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include INC_ARCH(exception.h)
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memory.h)

void init_ktcb_list(struct ktcb_list *ktcb_list)
{
	memset(ktcb_list, 0, sizeof(*ktcb_list));
	spin_lock_init(&ktcb_list->list_lock);
	link_init(&ktcb_list->list);
}

void tcb_init(struct ktcb *new)
{

	link_init(&new->task_list);
	mutex_init(&new->thread_control_lock);

	cap_list_init(&new->cap_list);
	cap_list_init(&new->tgroup_cap_list);
	cap_list_init(&new->pager_cap_list);

	/* Initialise task's scheduling state and parameters. */
	sched_init_task(new, TASK_PRIO_NORMAL);

	/* Initialise ipc waitqueues */
	spin_lock_init(&new->waitlock);
	waitqueue_head_init(&new->wqh_send);
	waitqueue_head_init(&new->wqh_recv);
	waitqueue_head_init(&new->wqh_pager);
}

struct ktcb *tcb_alloc_init(void)
{
	struct ktcb *tcb;
	struct task_ids ids;

	if (!(tcb = alloc_ktcb()))
		return 0;

	ids.tid = id_new(&kernel_resources.ktcb_ids);
	ids.tgid = L4_NILTHREAD;
	ids.spid = L4_NILTHREAD;

	set_task_ids(tcb, &ids);

	tcb_init(tcb);

	return tcb;
}

void tcb_delete(struct ktcb *tcb)
{
	/* Sanity checks first */
	BUG_ON(!is_page_aligned(tcb));
	BUG_ON(tcb->wqh_pager.sleepers > 0);
	BUG_ON(tcb->wqh_send.sleepers > 0);
	BUG_ON(tcb->wqh_recv.sleepers > 0);
	BUG_ON(!list_empty(&tcb->task_list));
	BUG_ON(!list_empty(&tcb->rq_list));
	BUG_ON(tcb->rq);
	BUG_ON(tcb->nlocks);
	BUG_ON(tcb->waiting_on);
	BUG_ON(tcb->wq);

	/*
	 * Take this lock as we may delete
	 * the address space as well
	 */
	address_space_reference_lock();
	BUG_ON(--tcb->space->ktcb_refs < 0);

	/* No refs left for the space, delete it */
	if (tcb->space->ktcb_refs == 0) {
		address_space_remove(tcb->space);
		address_space_delete(tcb->space);
	}

	address_space_reference_unlock();

	/* Deallocate tcb ids */
	id_del(&kernel_resources.ktcb_ids, tcb->tid);

	/* Free the tcb */
	free_ktcb(tcb);
}

struct ktcb *tcb_find_by_space(l4id_t spid)
{
	struct ktcb *task;

	spin_lock(&curcont->ktcb_list.list_lock);
	list_foreach_struct(task, &curcont->ktcb_list.list, task_list) {
		if (task->space->spid == spid) {
			spin_unlock(&curcont->ktcb_list.list_lock);
			return task;
		}
	}
	spin_unlock(&curcont->ktcb_list.list_lock);
	return 0;
}

struct ktcb *tcb_find(l4id_t tid)
{
	struct ktcb *task;

	if (current->tid == tid)
		return current;

	spin_lock(&curcont->ktcb_list.list_lock);
	list_foreach_struct(task, &curcont->ktcb_list.list, task_list) {
		if (task->tid == tid) {
			spin_unlock(&curcont->ktcb_list.list_lock);
			return task;
		}
	}
	spin_unlock(&curcont->ktcb_list.list_lock);
	return 0;
}

void ktcb_list_add(struct ktcb *new, struct ktcb_list *ktcb_list)
{
	spin_lock(&ktcb_list->list_lock);
	BUG_ON(!list_empty(&new->task_list));
	BUG_ON(!++ktcb_list->count);
	list_insert(&new->task_list, &ktcb_list->list);
	spin_unlock(&ktcb_list->list_lock);
}

void tcb_add(struct ktcb *new)
{
	struct container *c = new->container;

	spin_lock(&c->ktcb_list.list_lock);
	BUG_ON(!list_empty(&new->task_list));
	BUG_ON(!++c->ktcb_list.count);
	list_insert(&new->task_list, &c->ktcb_list.list);
	spin_unlock(&c->ktcb_list.list_lock);
}

void tcb_remove(struct ktcb *new)
{
	spin_lock(&curcont->ktcb_list.list_lock);
	BUG_ON(list_empty(&new->task_list));
	BUG_ON(--curcont->ktcb_list.count < 0);
	list_remove_init(&new->task_list);
	spin_unlock(&curcont->ktcb_list.list_lock);
}

/* Offsets for ktcb fields that are accessed from assembler */
unsigned int need_resched_offset = offsetof(struct ktcb, ts_need_resched);
unsigned int syscall_regs_offset = offsetof(struct ktcb, syscall_regs);

/*
 * Every thread has a unique utcb region that is mapped to its address
 * space as its context is loaded. The utcb region is a function of
 * this mapping and its offset that is reached via the KIP UTCB pointer
 */
void task_update_utcb(struct ktcb *task)
{
	/* Update the KIP pointer */
	kip.utcb = task->utcb_address;
}

/*
 * Checks whether a task's utcb is currently accessible by the kernel.
 * Returns an error if its not paged in yet, maps a non-current task's
 * utcb to current task for kernel-only access if it is unmapped.
 *
 * UTCB Mappings: The design is that each task maps its utcb with user
 * access, and any other utcb is mapped with kernel-only access privileges
 * upon an ipc that requires the kernel to access that utcb, in other
 * words foreign utcbs are mapped lazily.
 */
int tcb_check_and_lazy_map_utcb(struct ktcb *task)
{
	unsigned int phys;
	int ret;

	BUG_ON(!task->utcb_address);

	/*
	 * There you go:
	 *
	 * if task == current && not mapped, page-in, if not return -EFAULT
	 * if task != current && not mapped, return -EFAULT since can't page-in on behalf of it.
	 * if task != current && task mapped, but mapped != current mapped, map it, return 0
	 * if task != current && task mapped, but mapped == current mapped, return 0
	 */

	if (current == task) {
		/* Check own utcb, if not there, page it in */
		if ((ret = check_access(task->utcb_address, UTCB_SIZE,
					MAP_SVC_RW_FLAGS, 1)) < 0)
			return -EFAULT;
		else
			return 0;
	} else {
		/* Check another's utcb, but don't try to map in */
		if ((ret = check_access_task(task->utcb_address,
					     UTCB_SIZE,
					     MAP_SVC_RW_FLAGS, 0,
					     task)) < 0) {
			return -EFAULT;
		} else {
			/*
			 * Task has it mapped, map it to self
			 * unless they're identical
			 */
			if ((phys =
			     virt_to_phys_by_pgd(task->utcb_address,
						 TASK_PGD(task))) !=
			     virt_to_phys_by_pgd(task->utcb_address,
						 TASK_PGD(current))) {
				/*
				 * We have none or an old reference.
				 * Update it with privileged flags,
				 * so that only kernel can access.
				 */
				add_mapping_pgd(phys, task->utcb_address,
						page_align_up(UTCB_SIZE),
						MAP_SVC_RW_FLAGS,
						TASK_PGD(current));
			}
			BUG_ON(!phys);
		}
	}
#if 0
	/*
	 * FIXME:
	 *
	 * A task may have the utcb mapping of a destroyed thread
	 * at the given virtual address. This would silently be accepted
	 * as *mapped*. We need to ensure utcbs of destroyed tasks
	 * are cleared from all other task's page tables.
	 */
	if ((ret = check_access(task->utcb_address, UTCB_SIZE,
				MAP_SVC_RW_FLAGS, 0)) < 0) {
		/* Current task simply hasn't mapped its utcb */
		if (task == current) {
			return -EFAULT;
		} else {
			if (!(phys = virt_to_phys_by_pgd(task->utcb_address,
							 TASK_PGD(task)))) {
				/* Task hasn't mapped its utcb */
				return -EFAULT;
			} else {
				/*
				 * Task has utcb but it hasn't yet been mapped
				 * to current task with kernel access. Do it.
				 */
				add_mapping_pgd(phys, task->utcb_address,
						page_align_up(UTCB_SIZE),
						MAP_SVC_RW_FLAGS,
						TASK_PGD(current));
			}
		}
	}
#endif
	return 0;
}


