/*
 * Memory exception handling in process context.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include <l4/generic/thread.h>
#include <l4/api/thread.h>
#include <l4/generic/space.h>
#include <l4/generic/tcb.h>
#include <l4/lib/printk.h>
#include <l4/api/ipc.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include INC_PLAT(printascii.h)
#include INC_ARCH(exception.h)
#include INC_GLUE(memlayout.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(message.h)
#include INC_GLUE(ipc.h)
#include INC_SUBARCH(mm.h)

/* Abort debugging conditions */
//#define DEBUG_ABORTS
#if defined (DEBUG_ABORTS)
#define dbg_abort(...)	dprintk(__VA_ARGS__)
#else
#define dbg_abort(...)
#endif

struct ipc_state {
	u32 mr[MR_TOTAL];
	unsigned int flags;
};

void ipc_save_state(struct ipc_state *state)
{
	unsigned int *mr0_current = KTCB_REF_MR0(current);

	BUG_ON(!mr0_current);

	/* Save primary message registers */
	for (int i = 0; i < MR_TOTAL; i++)
		state->mr[i] = mr0_current[i];

	/* Save ipc flags */
	state->flags = tcb_get_ipc_flags(current);
}

void ipc_restore_state(struct ipc_state *state)
{
	unsigned int *mr0_current = KTCB_REF_MR0(current);

	BUG_ON(!mr0_current);

	/* Restore primary message registers */
	for (int i = 0; i < MR_TOTAL; i++)
		mr0_current[i] = state->mr[i];

	/* Restore ipc flags */
	tcb_set_ipc_flags(current, state->flags);
}

/* Send data fault ipc to the faulty task's pager */
void fault_ipc_to_pager(u32 faulty_pc, u32 fsr, u32 far)
{
	int err;

	/* mr[0] has the fault tag. The rest is the fault structure */
	u32 mr[MR_TOTAL] = {
		[MR_TAG] = L4_IPC_TAG_PFAULT,
		[MR_SENDER] = current->tid
	};

	fault_kdata_t *fault = (fault_kdata_t *)&mr[MR_UNUSED_START];

	/* Fill in fault information to pass over during ipc */
	fault->faulty_pc = faulty_pc;
	fault->fsr = fsr;
	fault->far = far;

	/* Write pte of the abort address, which is different on pabt/dabt */
	if (is_prefetch_abort(fsr))
		fault->pte = virt_to_pte(faulty_pc);
	else
		fault->pte = virt_to_pte(far);

	/*
	 * System calls save arguments (and message registers) on the kernel
	 * stack. They are then referenced from the caller's ktcb. Here, we
	 * forge a fault structure as if an ipc syscall has occured. Then
	 * the reference to the fault structure is set in the ktcb such that
	 * it lies on the mr0 offset when referred as the syscall context.
	 */

	/* Assign fault such that it overlaps as the MR0 reference in ktcb. */
	current->syscall_regs = (syscall_context_t *)
				((unsigned long)&mr[0] -
				 offsetof(syscall_context_t, r3));

	/* Set current flags to short ipc */
	tcb_set_ipc_flags(current, IPC_FLAGS_SHORT);

	/* Detect if a pager is self-faulting */
	if (current->tid == current->pagerid) {
		printk("Pager (%d) self-faulting. Exiting.\n",
		       current->tid);
		thread_destroy_self();
	}

	/* Send ipc to the task's pager */
	if ((err = ipc_sendrecv(current->pagerid,
				current->pagerid, 0)) < 0) {
		//printk("Thread (%d) faulted in kernel and its pager "
		//       "returned error (%d). Suspending.\n",
		//       current->tid, err);
		BUG_ON(current->nlocks);
		sched_suspend_sync();
	}
}

/*
 * When a task calls the kernel and the supplied user buffer is not mapped, the kernel
 * generates a page fault to the task's pager so that the pager can make the decision
 * on mapping the buffer. Remember that if a task maps its own user buffer to itself
 * this way, the kernel can access it, since it shares that task's page table.
 */
int pager_pagein_request(unsigned long addr, unsigned long size,
			 unsigned int flags)
{
	u32 abort = 0;
	unsigned long npages = __pfn(align_up(size, PAGE_SIZE));
	struct ipc_state ipc_state;

	set_abort_type(abort, ARM_DABT);

	// printk("%s: Kernel initiating paging-in requests\n", __FUNCTION__);

	/* Save current ipc state */
	ipc_save_state(&ipc_state);

	/* For every page to be used by the kernel send a page-in request */
	for (int i = 0; i < npages; i++)
		fault_ipc_to_pager(0, abort, addr + (i * PAGE_SIZE));

	/* Restore ipc state */
	ipc_restore_state(&ipc_state);

	return 0;
}

int check_aborts(u32 faulted_pc, u32 fsr, u32 far)
{
	int ret = 0;

	if (is_prefetch_abort(fsr)) {
		dbg_abort("Prefetch abort @ ", faulted_pc);
		return 0;
	}

	switch (fsr & ARM_FSR_MASK) {
	/* Aborts that are expected on page faults: */
	case DABT_PERM_PAGE:
		dbg_abort("Page permission fault @ ", far);
		ret = 0;
		break;
	case DABT_XLATE_PAGE:
		dbg_abort("Page translation fault @ ", far);
		ret = 0;
		break;
	case DABT_XLATE_SECT:
		dbg_abort("Section translation fault @ ", far);
		ret = 0;
		break;

	/* Aborts that can't be handled by a pager yet: */
	case DABT_TERMINAL:
		dprintk("Terminal fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_VECTOR:
		dprintk("Vector abort (obsolete!) @ ", far);
		ret = -EINVAL;
		break;
	case DABT_ALIGN:
		dprintk("Alignment fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_EXT_XLATE_LEVEL1:
		dprintk("External LVL1 translation fault @ ", far);
		ret = -EINVAL;
		break;
	case DABT_EXT_XLATE_LEVEL2:
		dprintk("External LVL2 translation fault @ ", far);
		ret = -EINVAL;
		break;
	case DABT_DOMAIN_SECT:
		dprintk("Section domain fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_DOMAIN_PAGE:
		dprintk("Page domain fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_PERM_SECT:
		dprintk("Section permission fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_EXT_LFETCH_SECT:
		dprintk("External section linefetch fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_EXT_LFETCH_PAGE:
		dprintk("Page perm fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_EXT_NON_LFETCH_SECT:
		dprintk("External section non-linefetch fault dabt @ ", far);
		ret = -EINVAL;
		break;
	case DABT_EXT_NON_LFETCH_PAGE:
		dprintk("External page non-linefetch fault dabt @ ", far);
		ret = -EINVAL;
		break;
	default:
		dprintk("FATAL: Unrecognised/Unknown data abort @ ", far);
		dprintk("FATAL: FSR code: ", fsr);
		ret = -EINVAL;
	}
	return ret;
}

/*
 * @r0: The address where the program counter was during the fault.
 * @r1: Contains the fault status register
 * @r2: Contains the fault address register
 */
void data_abort_handler(u32 faulted_pc, u32 fsr, u32 far)
{
	set_abort_type(fsr, ARM_DABT);

	dbg_abort("Data abort @ PC: ", faulted_pc);

	/* Check for more details */
	if (check_aborts(faulted_pc, fsr, far) < 0) {
		printascii("This abort can't be handled by any pager.\n");
		goto error;
	}
	if (KERN_ADDR(faulted_pc))
		goto error;

	/* This notifies the pager */
	fault_ipc_to_pager(faulted_pc, fsr, far);

	if (current->flags & TASK_SUSPENDING) {
		BUG_ON(current->nlocks);
		sched_suspend_sync();
	}

	return;

error:
	disable_irqs();
	dprintk("Unhandled data abort @ PC address: ", faulted_pc);
	dprintk("FAR:", far);
	dprintk("FSR:", fsr);
	printascii("Kernel panic.\n");
	printascii("Halting system...\n");
	while (1)
		;
}
void prefetch_abort_handler(u32 faulted_pc, u32 fsr, u32 far, u32 lr)
{
	set_abort_type(fsr, ARM_PABT);
	if (check_aborts(faulted_pc, fsr, far) < 0) {
		printascii("This abort can't be handled by any pager.\n");
		goto error;
	}

	if (KERN_ADDR(lr))
		goto error;
	fault_ipc_to_pager(faulted_pc, fsr, far);

	if (current->flags & TASK_SUSPENDING) {
		BUG_ON(current->nlocks);
		sched_suspend_sync();
	}
	return;

error:
	disable_irqs();
	dprintk("Unhandled prefetch abort @ address: ", faulted_pc);
	dprintk("FAR:", far);
	dprintk("FSR:", fsr);
	dprintk("LR:", lr);
	printascii("Kernel panic.\n");
	printascii("Halting system...\n");
	while (1)
		;
}

void dump_undef_abort(u32 undef_addr)
{
	dprintk("Undefined instruction at address: ", undef_addr);
	printascii("Halting system...\n");
}

extern int current_irq_nest_count;
/*
 * This is called right where the nest count is increased in case the nesting
 * is beyond the predefined max limit. It is another matter whether this
 * limit is enough to guarantee the kernel stack is not overflown.
 */
void irq_overnest_error(void)
{
	dprintk("Irqs nested beyond limit. Current count: ", current_irq_nest_count);
	printascii("Halting system...\n");
	while(1)
		;
}

