/*
 * Thread related system calls.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include <l4/generic/container.h>
#include <l4/api/thread.h>
#include <l4/api/syscall.h>
#include <l4/api/errno.h>
#include <l4/generic/tcb.h>
#include <l4/lib/idpool.h>
#include <l4/lib/mutex.h>
#include <l4/lib/wait.h>
#include <l4/generic/resource.h>
#include <l4/generic/capability.h>
#include INC_ARCH(asm.h)
#include INC_SUBARCH(mm.h)

int sys_thread_switch(void)
{
	schedule();
	return 0;
}

/*
 * This signals a thread so that the thread stops what it is
 * doing, and take action on the signal provided. Currently this
 * may be a suspension or an exit signal.
 */
int thread_signal(struct ktcb *task, unsigned int flags,
		  unsigned int task_state)
{
	int ret = 0;

	if (task->state == task_state)
		return 0;

	/* Signify we want to suspend the thread */
	task->flags |= flags;

	/* Wake it up if it's sleeping */
	wake_up_task(task, WAKEUP_INTERRUPT | WAKEUP_SYNC);

	/* Wait until task switches to desired state */
	WAIT_EVENT(&task->wqh_pager,
		   task->state == task_state, ret);

	return ret;
}

int thread_suspend(struct ktcb *task)
{
	return thread_signal(task, TASK_SUSPENDING, TASK_INACTIVE);
}

int thread_exit(struct ktcb *task)
{

	return thread_signal(task, TASK_EXITING, TASK_DEAD);
}

static inline int TASK_IS_CHILD(struct ktcb *task)
{
	return (((task) != current) &&
		((task)->pagerid == current->tid));
}

int thread_destroy_child(struct ktcb *task)
{
	thread_exit(task);

	tcb_remove(task);

	/* Wake up waiters */
	wake_up_all(&task->wqh_send, WAKEUP_INTERRUPT);
	wake_up_all(&task->wqh_recv, WAKEUP_INTERRUPT);

	BUG_ON(task->wqh_pager.sleepers > 0);
	BUG_ON(task->state != TASK_DEAD);

	tcb_delete(task);
	return 0;
}

int thread_destroy_children(void)
{
	struct ktcb *task, *n;

	spin_lock(&curcont->ktcb_list.list_lock);
	list_foreach_removable_struct(task, n,
				      &curcont->ktcb_list.list,
				      task_list) {
		if (TASK_IS_CHILD(task)) {
			spin_unlock(&curcont->ktcb_list.list_lock);
			thread_destroy_child(task);
			spin_lock(&curcont->ktcb_list.list_lock);
		}
	}
	spin_unlock(&curcont->ktcb_list.list_lock);
	return 0;

}

void thread_destroy_self(unsigned int exit_code)
{
	thread_destroy_children();
	current->exit_code = exit_code;
	sched_exit_sync();
}

int thread_wait(struct ktcb *task)
{
	int ret;

	/* Wait until task switches to desired state */
	WAIT_EVENT(&task->wqh_pager,
		   task->state == TASK_DEAD, ret);
	if (ret < 0)
		return ret;
	else
		return (int)task->exit_code;
}

int thread_destroy(struct ktcb *task, unsigned int exit_code)
{
	exit_code &= THREAD_EXIT_MASK;

	if (TASK_IS_CHILD(task))
		return thread_destroy_child(task);
	else if (task == current)
		thread_destroy_self(exit_code);
	return 0;
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

int thread_recycle(struct ktcb *task)
{
	int ret;

	if ((ret = thread_suspend(task)) < 0)
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

/* Runs a thread for the first time */
int thread_start(struct ktcb *task)
{
	if (!mutex_trylock(&task->thread_control_lock))
		return -EAGAIN;

	/* FIXME: Refuse to run dead tasks */

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
	if (flags & TC_NEW_SPACE) {
		BUG_ON(orig);
		new->context.spsr = ARM_MODE_USR;
		return 0;
	}

	BUG_ON(!orig);
	/*
	 * For duplicated threads pre-syscall context is saved on
	 * the kernel stack. We copy this context of original
	 * into the duplicate thread's current context structure
	 *
	 * No locks needed as the thread is not known to the system yet.
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

	/* Distribute original thread's ticks into two threads */
	new->ticks_left = (orig->ticks_left + 1) >> 1;
	if (!(orig->ticks_left >>= 1))
		orig->ticks_left = 1;

	return 0;
}

static inline void
thread_setup_new_ids(struct task_ids *ids, unsigned int flags,
		     struct ktcb *new, struct ktcb *orig)
{
	if (flags & TC_SHARE_GROUP)
		new->tgid = orig->tgid;
	else
		new->tgid = new->tid;

	/* Update ids to be returned back to caller */
	ids->tid = new->tid;
	ids->tgid = new->tgid;
}

int thread_setup_space(struct ktcb *tcb, struct task_ids *ids, unsigned int flags)
{
	struct address_space *space, *new;
	int ret = 0;

	address_space_reference_lock();

	if (flags & TC_SHARE_SPACE) {
		if (!(space = address_space_find(ids->spid))) {
			ret = -ESRCH;
			goto out;
		}
		address_space_attach(tcb, space);
	}
	if (flags & TC_COPY_SPACE) {
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
	if (flags & TC_NEW_SPACE) {
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
	struct ktcb *orig = 0;
	int err;

	/* Clear flags to just include creation flags */
	flags &= THREAD_CREATE_MASK;

	/* Can't have multiple space directives in flags */
	if ((flags & TC_SHARE_SPACE
	     & TC_COPY_SPACE & TC_NEW_SPACE) || !flags)
		return -EINVAL;

	/* Can't have multiple pager specifiers */
	if (flags & TC_SHARE_PAGER & TC_AS_PAGER)
		return -EINVAL;

	/* Can't request shared utcb or tgid without shared space */
	if (!(flags & TC_SHARE_SPACE)) {
		if ((flags & TC_SHARE_UTCB) ||
		    (flags & TC_SHARE_GROUP)) {
			return -EINVAL;
		}
	}

	if (!(new = tcb_alloc_init()))
		return -ENOMEM;

	/* Set up new thread space by using space id and flags */
	if ((err = thread_setup_space(new, ids, flags)) < 0)
		goto out_err;

	/* Obtain parent thread if there is one */
	if (flags & TC_SHARE_SPACE || flags & TC_COPY_SPACE) {
		if (!(orig = tcb_find(ids->tid))) {
			err = -EINVAL;
			goto out_err;
		}
	}

	/*
	 * Note this is a kernel-level relationship
	 * between the creator and the new thread.
	 *
	 * Any higher layer may define parent/child
	 * relationships between orig and new separately.
	 */
	if (flags & TC_AS_PAGER)
		new->pagerid = current->tid;
	else if (flags & TC_SHARE_PAGER)
		new->pagerid = current->pagerid;
	else
		new->pagerid = new->tid;

	//printk("Thread (%d) pager set as (%d)\n", new->tid, new->pagerid);

	/*
	 * Setup container-generic fields from current task
	 */
	new->container = current->container;

	/* Set up new thread context by using parent ids and flags */
	thread_setup_new_ids(ids, flags, new, orig);
	arch_setup_new_thread(new, orig, flags);

	tcb_add(new);

	//printk("%s: %d created: %d, %d, %d \n",
	//       __FUNCTION__, current->tid, ids->tid,
	//       ids->tgid, ids->spid);

	return 0;

out_err:
	/* Pre-mature tcb needs freeing by free_ktcb */
	free_ktcb(new);
	return err;
}

/*
 * Creates, destroys and modifies threads. Also implicitly creates an address
 * space for a thread that doesn't already have one, or destroys it if the last
 * thread that uses it is destroyed.
 */
int sys_thread_control(unsigned int flags, struct task_ids *ids)
{
	struct ktcb *task = 0;
	int err, ret = 0;

	if ((err = check_access((unsigned long)ids, sizeof(*ids),
				MAP_USR_RW_FLAGS, 1)) < 0)
		return err;

	if ((flags & THREAD_ACTION_MASK) != THREAD_CREATE)
		if (!(task = tcb_find(ids->tid)))
			return -ESRCH;

	if ((err = cap_thread_check(task, flags, ids)) < 0)
		return err;

	switch (flags & THREAD_ACTION_MASK) {
	case THREAD_CREATE:
		ret = thread_create(ids, flags);
		break;
	case THREAD_RUN:
		ret = thread_start(task);
		break;
	case THREAD_SUSPEND:
		ret = thread_suspend(task);
		break;
	case THREAD_DESTROY:
		ret = thread_destroy(task, flags);
		break;
	case THREAD_RECYCLE:
		ret = thread_recycle(task);
		break;
	case THREAD_WAIT:
		ret = thread_wait(task);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

