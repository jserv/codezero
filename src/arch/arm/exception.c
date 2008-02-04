/*
 * Debug print support for unexpected exceptions
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/scheduler.h>
#include <l4/generic/space.h>
#include <l4/generic/tcb.h>
#include <l4/lib/printk.h>
#include <l4/api/ipc.h>
#include <l4/api/errno.h>
#include INC_PLAT(printascii.h)
#include INC_ARCH(exception.h)
#include INC_GLUE(memlayout.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(utcb.h)
#include INC_SUBARCH(mm.h)

/*
 * NOTE: These are defined in libl4 headers for userspace. Syslib uses
 * these as conventional mr offsets to store ipc-related data commonly needed
 * for all ipc parties.
 */
#define	MR_TAG		0
#define MR_SENDERID	1
#define MR_UNUSED_START	2

/* Send data fault ipc to the faulty task's pager */
void fault_ipc_to_pager(u32 faulty_pc, u32 fsr, u32 far)
{
	/* mr[0] has the fault tag. The rest is the fault structure */
	u32 mr[MR_TOTAL] = { [MR_TAG] = L4_IPC_TAG_PFAULT,
			     [MR_SENDERID] = current->tid };
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
	 * stack. They are then referenced from the caller's ktcb. Here, the
	 * same ktcb reference is set to the fault data so it gives the effect
	 * as if the ipc to the pager has the fault data in the message
	 * registers saved on the kernel stack during an ipc syscall. Also this
	 * way fault does not need to modify the actual utcb MRs in userspace.
	 */

	/* Assign fault such that it overlaps as the MR0 reference in ktcb. */
	current->syscall_regs = (syscall_args_t *)
				((unsigned long)&mr[0] -
				 offsetof(syscall_args_t, r3));

	/* Send ipc to the task's pager */
	ipc_sendrecv(current->pagerid, current->pagerid);

	/*
	 * Pager is now notified and handling the fault. We now sleep on
	 * another queue.
	 */
}

int check_aborts(u32 faulted_pc, u32 fsr, u32 far)
{
	int ret = 0;

	if (is_prefetch_abort(fsr)) {
		dprintk("Prefetch abort @ ", faulted_pc);
		return 0;
	}

	switch (fsr & ARM_FSR_MASK) {
	/* Aborts that are expected on page faults: */
	case DABT_PERM_PAGE:
		dprintk("Page permission fault @ ", far);
		ret = 0;
		break;
	case DABT_XLATE_PAGE:
		dprintk("Page translation fault @ ", far);
		ret = 0;
		break;
	case DABT_XLATE_SECT:
		dprintk("Section translation fault @ ", far);
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
	dprintk("Data abort @ PC: ", faulted_pc);
	if (check_aborts(faulted_pc, fsr, far) < 0) {
		printascii("This abort can't be handled by any pager.\n");
		goto error;
	}
	if (KERN_ADDR(faulted_pc))
		goto error;

	/* This notifies the pager */
	fault_ipc_to_pager(faulted_pc, fsr, far);

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

void prefetch_abort_handler(u32 faulted_pc, u32 fsr, u32 far)
{
	set_abort_type(fsr, ARM_PABT);
	if (check_aborts(faulted_pc, fsr, far) < 0) {
		printascii("This abort can't be handled by any pager.\n");
		goto error;
	}
	fault_ipc_to_pager(faulted_pc, fsr, far);
	return;

error:
	disable_irqs();
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

