/*
 * Some ktcb related data
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/container.h>
#include <l4/generic/preempt.h>
#include <l4/generic/space.h>
#include <l4/lib/idpool.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include INC_ARCH(exception.h)
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memory.h)

/* ID pools for threads and spaces. */
struct id_pool *thread_id_pool;
struct id_pool *space_id_pool;


void init_ktcb_list(struct ktcb_list *ktcb_list)
{
	memset(ktcb_list, 0, sizeof(*ktcb_list));
	spin_lock_init(&ktcb_list->list_lock);
	link_init(&ktcb_list->list);
}

void tcb_init(struct ktcb *new)
{
	new->tid = id_new(&kernel_container.ktcb_ids);
	new->tgid = new->tid;

	link_init(&new->task_list);
	mutex_init(&new->thread_control_lock);

	/* Initialise task's scheduling state and parameters. */
	sched_init_task(new, TASK_PRIO_NORMAL);

	/* Initialise ipc waitqueues */
	spin_lock_init(&new->waitlock);
	waitqueue_head_init(&new->wqh_send);
	waitqueue_head_init(&new->wqh_recv);
	waitqueue_head_init(&new->wqh_pager);
}

struct ktcb *tcb_alloc(void)
{
	return alloc_ktcb();
}

struct ktcb *tcb_alloc_init(void)
{
	struct ktcb *tcb;

	if (!(tcb = tcb_alloc()))
		return 0;

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
	id_del(thread_id_pool, tcb->tid);

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

void tcb_add(struct ktcb *new)
{
	spin_lock(&curcont->ktcb_list.list_lock);
	BUG_ON(!list_empty(&new->task_list));
	BUG_ON(!++curcont->ktcb_list.count);
	list_insert(&new->task_list, &curcont->ktcb_list.list);
	spin_unlock(&curcont->ktcb_list.list_lock);
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
void task_update_utcb(struct ktcb *cur, struct ktcb *next)
{
	/* Update the KIP pointer */
	kip.utcb = next->utcb_address;
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
	return 0;
}


