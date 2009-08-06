/*
 * System Calls
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/tcb.h>
#include <l4/generic/resource.h>
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/api/space.h>
#include <l4/api/ipc.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include <l4/api/exregs.h>
#include INC_API(syscall.h)
#include INC_ARCH(exception.h)

void print_syscall_context(struct ktcb *t)
{
	syscall_context_t *r = t->syscall_regs;

	printk("Thread id: %d registers: 0x%x, 0x%x, 0x%x, 0x%x, "
	       "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
	       t->tid, r->spsr, r->r0, r->r1, r->r2, r->r3, r->r4,
	       r->r5, r->r6, r->r7, r->r8, r->sp_usr, r->lr_usr);
}

/* Copy each register to task's context if its valid bit is set */
void do_exchange_registers(struct ktcb *task, struct exregs_data *exregs)
{
	task_context_t *context = &task->context;

	/*
	 * NOTE:
	 * We don't care if register values point at invalid addresses
	 * since memory protection would prevent any kernel corruption.
	 * We do however, make sure spsr is not modified
	 */

	/* Check register valid bit and copy registers */
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r0))
		context->r0 = exregs->context.r0;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r1))
		context->r1 = exregs->context.r1;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r2))
		context->r2 = exregs->context.r2;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r3))
		context->r3 = exregs->context.r3;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r4))
		context->r4 = exregs->context.r4;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r5))
		context->r5 = exregs->context.r5;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r6))
		context->r6 = exregs->context.r6;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r7))
		context->r7 = exregs->context.r7;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r8))
		context->r8 = exregs->context.r8;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r9))
		context->r9 = exregs->context.r9;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r10))
		context->r10 = exregs->context.r10;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r11))
		context->r11 = exregs->context.r11;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, r12))
		context->r12 = exregs->context.r12;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, sp))
		context->sp = exregs->context.sp;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, lr))
		context->lr = exregs->context.lr;
	if (exregs->valid_vect & FIELD_TO_BIT(exregs_context_t, pc))
		context->pc = exregs->context.pc;

	/* Set thread's pager if one is supplied */
	if (exregs->flags & EXREGS_SET_PAGER)
		task->pagerid = exregs->pagerid;

	/* Set thread's utcb if supplied */
	if (exregs->flags & EXREGS_SET_UTCB)
		task->utcb_address = exregs->utcb_address;
}

/*
 * exchange_registers()
 *
 * This call is used by the pagers to set (and in the future read)
 * the register context of a thread. The thread's registers can be
 * set only when the thread is in user mode. A newly created thread
 * that is the copy of another thread (forked or cloned) will also
 * be given its user mode context on the first chance to execute so
 * such threads can also be modified by this call before execution.
 *
 * A thread executing in the kernel cannot be modified since this
 * would compromise the kernel. Also the thread must be in suspended
 * condition so that the scheduler does not execute it as we modify
 * its context.
 */
int sys_exchange_registers(struct exregs_data *exregs, l4id_t tid)
{
	int err = 0;
	struct ktcb *task;

	/* Find tcb from its list */
	if (!(task = tcb_find(tid)))
		return -ESRCH;

	/*
	 * This lock ensures task is not
	 * inadvertently resumed by a syscall
	 */
	if (!mutex_trylock(&task->thread_control_lock))
		return -EAGAIN;

	/* Now check that the task is suspended */
	if (task->state != TASK_INACTIVE) {
		err = -EACTIVE;
		goto out;
	}

#if 0
	A suspended thread implies it cannot do any harm
	even if it is in kernel mode.
	/*
	 * The thread must be in user mode for its context
	 * to be modified.
	 */
	if (!TASK_IN_USER(task)) {
		err = -EPERM;
		goto out;
	}
#endif

	/* Check UTCB is in valid range */
	if (exregs->flags & EXREGS_SET_UTCB &&
	    !(exregs->utcb_address >= UTCB_AREA_START &&
	    exregs->utcb_address < UTCB_AREA_END))
		return -EINVAL;

	/* Copy registers */
	do_exchange_registers(task, exregs);

out:
	/* Unlock and return */
	mutex_unlock(&task->thread_control_lock);
	return err;
}

int sys_schedule(void)
{
	printk("(SVC) %s called. Tid (%d)\n", __FUNCTION__, current->tid);
	return 0;
}

int sys_space_control(void)
{
	return -ENOSYS;
}

int sys_getid(struct task_ids *ids)
{
	struct ktcb *this = current;

	ids->tid = this->tid;
	ids->spid = this->space->spid;
	ids->tgid = this->tgid;

	return 0;
}

int sys_container_control(unsigned int req, unsigned int flags, void *userbuf)
{
	return 0;
}


