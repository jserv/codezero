/*
 * Userspace irq handling and management
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/api/irq.h>
#include <l4/api/errno.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/thread.h>
#include <l4/generic/capability.h>
#include <l4/generic/irq.h>
#include <l4/generic/tcb.h>
#include INC_GLUE(message.h)
#include <l4/lib/wait.h>

/*
 * Default function that handles userspace
 * threaded irqs. Increases irq count and wakes
 * up any waiters.
 *
 * The increment is a standard read/update/write, and
 * it is atomic due to multiple reasons:
 *
 * - We are atomic with regard to the task because we are
 *   in irq context. Task cannot run during the period where
 *   we read update and write the value.
 *
 * - The task does an atomic swap on reads, writing 0 to the
 *   location in the same instruction as it reads it. As a
 *   result task updates on the slot are atomic, and cannot
 *   interfere with the read/update/write of the irq.
 *
 * - Recursive irqs are enabled, but we are also protected
 *   from them because the current irq number is masked.
 *
 *   FIXME: Instead of UTCB, do it by incrementing a semaphore.
 */
int irq_thread_notify(struct irq_desc *desc)
{
	struct utcb *utcb;
	int err;

	/* Make sure irq thread's utcb is mapped */
	if ((err = tcb_check_and_lazy_map_utcb(desc->task,
					       0)) < 0) {
		printk("%s: Irq occured but registered task's utcb "
		       "is inaccessible without a page fault. "
		       "task id=0x%x err=%d\n"
		       "Destroying task.", __FUNCTION__,
		       desc->task->tid, err);
		thread_destroy(desc->task);
		/* FIXME: Deregister and disable irq as well */
	}

	/* Get thread's utcb */
	utcb = (struct utcb *)desc->task->utcb_address;

	/* Atomic increment (See above comments) with no wraparound */
	if (utcb->notify[desc->task_notify_slot] != TASK_NOTIFY_MAX)
		utcb->notify[desc->task_notify_slot]++;

	/* Async wake up any waiter irq threads */
	wake_up(&desc->task->wqh_notify, WAKEUP_ASYNC | WAKEUP_IRQ);

	return 0;
}

/*
 * Register the given globally unique irq number with the
 * current thread with given flags
 */
int irq_control_register(struct ktcb *task, int slot, l4id_t irqnum,
			 unsigned long device_virtual,
			 struct capability *devcap)
{
	int err;

	/*
	 * Check that utcb memory accesses won't fault us.
	 *
	 * We make sure that the irq registrar has its utcb mapped,
	 * so that if an irq occurs, the utcb address must be at
	 * least readily available to be copied over to the page
	 * tables of the task runnable at the time of the irq.
	 */
	if ((err = tcb_check_and_lazy_map_utcb(current, 1)) < 0)
		return err;

	/* Register the irq for thread notification */
	if ((err = irq_register(current, slot, irqnum, devcap)) < 0)
		return err;

	return 0;
}

/*
 * Register/deregister device irqs. Optional synchronous and
 * asynchronous irq handling.
 */
int sys_irq_control(unsigned int req, unsigned int flags, l4id_t irqno)
{
	/* Currently a task is allowed to register only for itself */
	struct ktcb *task = current;
	struct capability *devcap;
	int err;

	if ((err = cap_irq_check(task, req, flags, irqno, &devcap)) < 0)
		return err;

	switch (req) {
	case IRQ_CONTROL_REGISTER:
		if ((err = irq_control_register(task, slot, flags,
						irqno, devcap)) < 0)
			return err;
	default:
			return -EINVAL;
	}
	return 0;
}

