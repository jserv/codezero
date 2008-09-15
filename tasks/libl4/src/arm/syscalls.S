/*
 * Userspace system call interface.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4lib/arch/asm.h>
#include <l4lib/arch/utcb.h>
#include <l4/generic/space.h>
#include <l4/macros.h>
#include INC_GLUE(message.h)

	.macro	utcb_address	rx
		ldr	\rx, =utcb
	.endm

BEGIN_PROC(l4_thread_switch)
	ldr	r12, =__l4_thread_switch
	ldr	pc, [r12]	@ Jump into the SWI. Kernel returns to LR_USR, which is the caller.
END_PROC(l4_thread_switch)

/*
 * The syscall returns process ids. This function saves the returned values in the
 * arguments passed by reference. @r0 = struct task_ids *
 */
BEGIN_PROC(l4_getid)
	ldr	r12, =__l4_getid	@ See l4_kdata_read for why its so simple.
	ldr	pc, [r12]		@ Return.
END_PROC(l4_getid)

/*
 * Reads data from the kernel into given buffer. Data is defined by request descriptor.
 * @r0 = request descriptor, @r1 = buffer address.
 */
BEGIN_PROC(l4_kread)
	ldr	r12, =__l4_kread
	ldr	pc, [r12]		@ Jump into the SWI
		/*
		 * The LR_USR points at the return address of this function. The system
		 * call return path directly jumps to LR_USR so we don't even need a
		 * return instruction here.
		 */
END_PROC(l4_kread)

/*
 * For clone() we need special assembler handling
 * Same signature as ipc(): @r0 = to, @r1 = from
 *
 * NOTE: Note that this breaks l4 system call interface,
 * this should be moved elsewhere and modified using existing l4 mechanisms.
 */
BEGIN_PROC(arch_clone)
	stmfd	sp!, {r4-r8,lr}		@ Save context.
	utcb_address r12		@ Get utcb address.
	ldmia	r12!, {r3-r8}		@ Load 6 Message registers from utcb. MR0-MR5
	ldr	r12, =__l4_ipc
	mov	lr, pc
	ldr	pc, [r12]		@ Perform the ipc()

	/*
	 * At this moment:
	 * - MR_RETURN tells us whether we are parent or child (or have failed).
	 * - Child has new SP set, with |func_ptr|arg1|{End of stack}SP<-| on stack.
	 * - Child needs exit logic when its function is finished.
	 */
	cmp	r0, #0			@ Check ipc success
	blt	ipc_failed
	cmp	MR_RETURN_REGISTER, #0	@ Check ipc return register MR_RETURN.
	blt	clone_failed		@ Ipc was ok but clone() failed.
	bgt	parent_return		@ It has child pid, goto parent return.
child:
	ldr	r0, [sp, #-4]!		@ Load child's first argument.
	mov	lr, pc			@ Save return address
	ldr	pc, [sp, #-4]!		@ Load function pointer from stack
child_exit:
	b	child_exit		@ We infinitely loop for now.

	@ Return with normal ipc return sequence
parent_return:
clone_failed:
ipc_failed:
	utcb_address r12		@ Get utcb
	stmia	r12, {r3-r8}		@ Store mrs.
	ldmfd	sp!, {r4-r8,pc}		@ Return restoring pc and context.
END_PROC(arch_clone)

/*
 * Inter-process communication. Loads message registers as arguments before the call,
 * and stores them as results after the call. @r0 = to, @r1 = from.
 */
BEGIN_PROC(l4_ipc)
	stmfd	sp!, {r4-r8,lr}		@ Save context.
	utcb_address r12		@ Get utcb address.
	ldmia	r12!, {r3-r8}		@ Load 6 Message registers from utcb. MR0-MR5
	ldr	r12, =__l4_ipc
	mov	lr, pc
	ldr	pc, [r12]
	utcb_address r12		@ Get utcb address.
	stmia	r12, {r3-r8}		@ Store 6 Message registers to utcb. MR0-MR5
	ldmfd	sp!, {r4-r8,pc}		@ Return restoring pc, and context.
END_PROC(l4_ipc)

/*
 * System call that maps an area of memory into the given address space.
 * @r0 = physical address, @r1 = virtual address, @r2 = map size in pages,
 * @r3 = map flags, @r4 = The tgid of the address space to map.
 */
BEGIN_PROC(l4_map)
	stmfd	sp!, {r4, lr}
	ldr	r4, [sp, #8]		@ FIXME: Is this right?
	ldr	r12, =__l4_map
	mov	lr, pc			@ We must return here to restore r4.
	ldr	pc, [r12]
	ldmfd	sp!, {r4, pc}
END_PROC(l4_map)

/*
 * System call that unmaps an area of memory into the given address space.
 * @r0 = virtual, @r1 = pages,  @r2 = tid of address space to unmap
 */
BEGIN_PROC(l4_unmap)
	stmfd	sp!, {lr}
	ldr	r12, =__l4_unmap
	mov	lr, pc
	ldr	pc, [r12]
	ldmfd	sp!, {pc}	@ Restore original lr and return.
END_PROC(l4_unmap)

/*
 * System call that grants a set of pages to the kernel.
 * @r0 = physical pfn, @r1 = number of pages, @r2 = whether to
 * grant or reclaim kernel memory. grant = 1, reclaim = 0.
 */
BEGIN_PROC(l4_kmem_control)
	stmfd	sp!, {lr}
	ldr	r12, =__l4_kmem_control
	mov	lr, pc
	ldr	pc, [r12]
	ldmfd	sp!, {pc}	@ Restore original lr and return.
END_PROC(l4_kmem_control)

/*
 * System call that gets or sets the time info structure.
 * @r0 = ptr to time structure @r1 = set or get. set = 1, get = 0.
 */
BEGIN_PROC(l4_time)
	stmfd	sp!, {lr}
	ldr	r12, =__l4_time
	mov	lr, pc
	ldr	pc, [r12]
	ldmfd	sp!, {pc}	@ Restore original lr and return.
END_PROC(l4_time)

/*
 * System call that controls thread creation, destruction and modification.
 * @r0 = thread action, @r1 = &ids
 */
BEGIN_PROC(l4_thread_control)
	stmfd	sp!, {lr}
	ldr	r12, =__l4_thread_control
	mov	lr, pc
	ldr	pc, [r12]
	ldmfd	sp!, {pc}	@ Restore original lr and return.
END_PROC(l4_thread_control)

/*
 * System call that modifies ipc blocked sender lists of receivers.
 * @r0 =  Action (e.g. block/unblock), @r1 = sender id, @r2 = sender tag
 */
BEGIN_PROC(l4_ipc_control)
	stmfd	sp!, {lr}
	ldr	r12, =__l4_ipc_control
	mov	lr, pc
	ldr	pc, [r12]
	ldmfd	sp!, {pc}	@ Restore original lr and return.
END_PROC(l4_ipc_control)

/*
 * Manipulates address spaces, e.g. sets up shared memory areas between threads
 * @r0 = operation code, @r1 = struct shm_kdata *kdata
 */
BEGIN_PROC(l4_space_control)
	stmfd	sp!, {lr}
	ldr	r12, =__l4_space_control
	mov	lr, pc
	ldr	pc, [r12]
	ldmfd	sp!, {pc}	@ Restore original lr and return.
END_PROC(l4_space_control)

/*
 * Sets registers of a thread and its pager.
 * @r0 = ptr to exregs_data structure, @r1 = tid of thread.
 */
BEGIN_PROC(l4_exchange_registers)
	stmfd	sp!, {lr}
	ldr	r12, =__l4_exchange_registers
	mov	lr, pc
	ldr	pc, [r12]
	ldmfd	sp!, {pc}	@ Restore original lr and return.
END_PROC(l4_exchange_registers)

