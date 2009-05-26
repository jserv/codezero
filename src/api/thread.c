/*
 * Thread related system calls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include <l4/api/thread.h>
#include <l4/api/syscall.h>
#include <l4/api/errno.h>
#include <l4/generic/tcb.h>
#include <l4/lib/idpool.h>
#include <l4/lib/mutex.h>
#include <l4/lib/wait.h>
#include <l4/generic/pgalloc.h>
#include INC_ARCH(asm.h)
#include INC_SUBARCH(mm.h)

int sys_thread_switch(syscall_context_t *regs)
{
	schedule();
	return 0;
}

/*
 * This suspends a thread which is in either suspended,
 * sleeping or runnable state.
 */
int thread_suspend(struct task_ids *ids)
{
	struct ktcb *task;
	int ret = 0;

	if (!(task = tcb_find(ids->tid)))
		return -ESRCH;

	if (task->state == TASK_INACTIVE)
		return 0;

	/* Signify we want to suspend the thread */
	task->flags |= TASK_SUSPENDING;

	/* Wake it up if it's sleeping */
	wake_up_task(task, WAKEUP_INTERRUPT | WAKEUP_SYNC);

	/* Wait until task suspends itself */
	WAIT_EVENT(&task->wqh_pager,
		   task->state == TASK_INACTIVE, ret);

	return ret;
}

int arch_clear_thread(struct ktcb *tcb)
{
	/* Remove from the global list */
	tcb_remove(tcb);

	/* Sanity checks */
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

	/* Reinitialise the context */
	memset(&tcb->context, 0, sizeof(tcb->context));
	tcb->context.spsr = ARM_MODE_USR;

	/* Clear the page tables */
	remove_mapping_pgd_all_user(TASK_PGD(tcb));

	/* Reinitialize all other fields */
	tcb_init(tcb);

	/* Add back to global list */
	tcb_add(tcb);

	return 0;
}

int thread_recycle(struct task_ids *ids)
{
	struct ktcb *task;
	int ret;

	if (!(task = tcb_find(ids->tid)))
		return -ESRCH;

	if ((ret = thread_suspend(ids)) < 0)
		return ret;

	/*
	 * If there are any sleepers on any of the task's
	 * waitqueues, we need to wake those tasks up.
	 */
	wake_up_all(&task->wqh_send, 0);
	wake_up_all(&task->wqh_recv, 0);

	/*
	 * The thread cannot have a pager waiting for it
	 * since we ought to be the pager.
	 */
	BUG_ON(task->wqh_pager.sleepers > 0);

	/* Clear the task's tcb */
	arch_clear_thread(task);

	return 0;
}

int thread_destroy(struct task_ids *ids)
{
	struct ktcb *task;
	int ret;

	if (!(task = tcb_find(ids->tid)))
		return -ESRCH;

	if ((ret = thread_suspend(ids)) < 0)
		return ret;

	/* Remove tcb from global list so any callers will get -ESRCH */
	tcb_remove(task);

	/*
	 * If there are any sleepers on any of the task's
	 * waitqueues, we need to wake those tasks up.
	 */
	wake_up_all(&task->wqh_send, 0);
	wake_up_all(&task->wqh_recv, 0);

	/* We can now safely delete the task */
	tcb_delete(task);

	return 0;
}

int thread_resume(struct task_ids *ids)
{
	struct ktcb *task;

	if (!(task = tcb_find(ids->tid)))
		return -ESRCH;

	if (!mutex_trylock(&task->thread_control_lock))
		return -EAGAIN;

	/* Put task into runqueue as runnable */
	sched_resume_async(task);

	/* Release lock and return */
	mutex_unlock(&task->thread_control_lock);
	return 0;
}

/* Runs a thread for the first time */
int thread_start(struct task_ids *ids)
{
	struct ktcb *task;

	if (!(task = tcb_find(ids->tid)))
       		return -ESRCH;

	if (!mutex_trylock(&task->thread_control_lock))
		return -EAGAIN;

	/* Notify scheduler of task resume */
	sched_resume_async(task);

	/* Release lock and return */
	mutex_unlock(&task->thread_control_lock);
	return 0;
}

int arch_setup_new_thread(struct ktcb *new, struct ktcb *orig,
			  unsigned int flags)
{
	/* New threads just need their mode set up */
	if ((flags & THREAD_CREATE_MASK) == THREAD_NEW_SPACE) {
		BUG_ON(orig);
		new->context.spsr = ARM_MODE_USR;
		return 0;
	}

	/*
	 * For duplicated threads pre-syscall context is saved on
	 * the kernel stack. We copy this context of original
	 * into the duplicate thread's current context structure
	 *
	 * We don't lock for context modification because the
	 * thread is not known to the system yet.
	 */
	BUG_ON(!(new->context.spsr = orig->syscall_regs->spsr)); /* User mode */
	new->context.r0 = orig->syscall_regs->r0;
	new->context.r1 = orig->syscall_regs->r1;
	new->context.r2 = orig->syscall_regs->r2;
	new->context.r3 = orig->syscall_regs->r3;
	new->context.r4 = orig->syscall_regs->r4;
	new->context.r5 = orig->syscall_regs->r5;
	new->context.r6 = orig->syscall_regs->r6;
	new->context.r7 = orig->syscall_regs->r7;
	new->context.r8 = orig->syscall_regs->r8;
	new->context.r9 = orig->syscall_regs->r9;
	new->context.r10 = orig->syscall_regs->r10;
	new->context.r11 = orig->syscall_regs->r11;
	new->context.r12 = orig->syscall_regs->r12;
	new->context.sp = orig->syscall_regs->sp_usr;
	/* Skip lr_svc since it's not going to be used */
	new->context.pc = orig->syscall_regs->lr_usr;

	/* Copy other relevant fields from original ktcb */
	new->pagerid = orig->pagerid;

	/* Distribute original thread's ticks into two threads */
	new->ticks_left = orig->ticks_left / 2;
	orig->ticks_left /= 2;

	return 0;
}

/*
 * Sets up the thread, thread group and space id of newly created thread
 * according to supplied flags.
 */
int thread_setup_new_ids(struct task_ids *ids, unsigned int flags,
			 struct ktcb *new, struct ktcb *orig)
{
	/* Allocate a new id */
	ids->tid = id_new(thread_id_pool);

	/*
	 * If thread space is new or copied,
	 * thread gets same group id as its thread id
	 */
	if (flags == THREAD_NEW_SPACE ||
	    flags == THREAD_COPY_SPACE)
		ids->tgid = ids->tid;

	/*
	 * If the tgid of original thread is supplied, that implies the
	 * new thread wants to be in the same group, and we leave it as
	 * it is. Otherwise the thread gets the same group id as its
	 * unique thread id.
	 */
	if (flags == THREAD_SAME_SPACE && ids->tgid != orig->tgid)
		ids->tgid = ids->tid;

	/* Set all ids */
	set_task_ids(new, ids);

	return 0;
}

int thread_setup_space(struct ktcb *tcb, struct task_ids *ids, unsigned int flags)
{
	struct address_space *space, *new;
	int ret = 0;

	address_space_reference_lock();

	if (flags == THREAD_SAME_SPACE) {
		if (!(space = address_space_find(ids->spid))) {
			ret = -ESRCH;
			goto out;
		}
		address_space_attach(tcb, space);
	}
	if (flags == THREAD_COPY_SPACE) {
		if (!(space = address_space_find(ids->spid))) {
			ret = -ESRCH;
			goto out;
		}
		if (IS_ERR(new = address_space_create(space))) {
			ret = (int)new;
			goto out;
		}
		/* New space id to be returned back to caller */
		ids->spid = new->spid;
		address_space_attach(tcb, new);
		address_space_add(new);
	}
	if (flags == THREAD_NEW_SPACE) {
		if (IS_ERR(new = address_space_create(0))) {
			ret = (int)new;
			goto out;
		}
		/* New space id to be returned back to caller */
		ids->spid = new->spid;
		address_space_attach(tcb, new);
		address_space_add(new);
	}

out:
	address_space_reference_unlock();
	return ret;
}

int thread_create(struct task_ids *ids, unsigned int flags)
{
	struct ktcb *new;
	struct ktcb *parent;
	int err;

	flags &= THREAD_CREATE_MASK;
	parent = 0;

	if (!(new = tcb_alloc_init()))
		return -ENOMEM;

	/* Set up new thread space by using space id and flags */
	if ((err = thread_setup_space(new, ids, flags)) < 0)
		goto out_err;

	/* Obtain parent thread if there is one */
	if (flags == THREAD_SAME_SPACE ||
	    flags == THREAD_COPY_SPACE) {
		if (!(parent = tcb_find(ids->tid))) {
			err = -EINVAL;
			goto out_err;
		}
	}

	/* Set up new thread context by using parent ids and flags */
	thread_setup_new_ids(ids, flags, new, parent);
	arch_setup_new_thread(new, parent, flags);

	tcb_add(new);

	return 0;

out_err:
	/* Pre-mature tcb needs freeing by free_page */
	free_page(new);
	return err;
}

/*
 * Creates, destroys and modifies threads. Also implicitly creates an address
 * space for a thread that doesn't already have one, or destroys it if the last
 * thread that uses it is destroyed.
 */
int sys_thread_control(syscall_context_t *regs)
{
	int ret = 0;
	unsigned int flags = regs->r0;
	struct task_ids *ids = (struct task_ids *)regs->r1;

	switch (flags & THREAD_ACTION_MASK) {
	case THREAD_CREATE:
		ret = thread_create(ids, flags);
		break;
	case THREAD_RUN:
		ret = thread_start(ids);
		break;
	case THREAD_SUSPEND:
		ret = thread_suspend(ids);
		break;
	case THREAD_RESUME:
		ret = thread_resume(ids);
		break;
	case THREAD_DESTROY:
		ret = thread_destroy(ids);
		break;
	case THREAD_RECYCLE:
		ret = thread_recycle(ids);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

