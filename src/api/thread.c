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
 * This suspends a thread which is in either suspended,
 * sleeping or runnable state. `flags' field specifies an additional
 * status for the thread, that implies an additional action as well
 * as suspending. For example, a TASK_EXITING flag ensures the task
 * is moved to a zombie queue during suspension.
 *
 * Why no race?
 *
 * There is no race between the pager setting TASK_SUSPENDING,
 * and waiting for TASK_INACTIVE non-atomically because the target
 * task starts suspending only when it sees TASK_SUSPENDING set and
 * it only wakes up the pager after it has switched state to
 * TASK_INACTIVE.
 *
 * If the pager hasn't come to wait_event() and the wake up call is
 * already gone, the state is already TASK_INACTIVE so the pager
 * won't sleep at all.
 */
int thread_suspend(struct ktcb *task, unsigned int flags)
{
	int ret = 0;

	if (task->state == TASK_INACTIVE)
		return 0;

	/* Signify we want to suspend the thread */
	task->flags |= TASK_SUSPENDING | flags;

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

int thread_recycle(struct ktcb *task)
{
	int ret;

	if ((ret = thread_suspend(task, 0)) < 0)
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

void thread_destroy_current();

int thread_destroy(struct ktcb *task)
{
	int ret;

	/*
	 * Pager destroying itself
	 */
	if (task == current) {
		thread_destroy_current();

		/* It should not return */
		BUG();
	}

	if ((ret = thread_suspend(task, 0)) < 0)
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

void task_make_zombie(struct ktcb *task)
{
	/* Remove from its list, callers get -ESRCH */
	tcb_remove(task);

	/*
	 * If there are any sleepers on any of the task's
	 * waitqueues, we need to wake those tasks up.
	 */
	wake_up_all(&task->wqh_send, 0);
	wake_up_all(&task->wqh_recv, 0);

	BUG_ON(!(task->flags & TASK_EXITING));

	/* Add to zombie list, to be destroyed later */
	ktcb_list_add(task, &kernel_resources.zombie_list);
}

/*
 * Pagers destroy themselves either by accessing an illegal
 * address or voluntarily. All threads managed also get
 * destroyed.
 */
void thread_destroy_current(void)
{
	struct ktcb *task, *n;

	/* Signal death to all threads under control of this pager */
	spin_lock(&curcont->ktcb_list.list_lock);
	list_foreach_removable_struct(task, n,
				      &curcont->ktcb_list.list,
				      task_list) {
		if (task->tid == current->tid ||
		    task->pagerid != current->tid)
			continue;
		spin_unlock(&curcont->ktcb_list.list_lock);

		/* Here we wait for each to die */
		thread_suspend(task, TASK_EXITING);
		spin_lock(&curcont->ktcb_list.list_lock);
	}
	spin_unlock(&curcont->ktcb_list.list_lock);

	/* Destroy all children */
	mutex_lock(&current->task_dead_mutex);
	list_foreach_removable_struct(task, n,
				      &current->task_dead_list,
				      task_list) {
		tcb_delete(task);
	}
	mutex_unlock(&current->task_dead_mutex);

	/* Destroy self */
	sched_die_sync();
}

/* Runs a thread for the first time */
int thread_start(struct ktcb *task)
{
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
	new->ticks_left = orig->ticks_left / 2;
	orig->ticks_left /= 2;

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
		ret = thread_suspend(task, flags);
		break;
	case THREAD_DESTROY:
		ret = thread_destroy(task);
		break;
	case THREAD_RECYCLE:
		ret = thread_recycle(task);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

