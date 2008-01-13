/*
 * ARM-specific system call details.
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */

#ifndef __ARM_GLUE_SYSCALL_H__
#define __ARM_GLUE_SYSCALL_H__

/* Only specific call is the trap that gives back the kip address
 * from which other system calls can be discovered. */
#define L4_TRAP_KIP			0xB4

/* Used in the kernel to refer to virtual address of this page.
 * User space discovers it from the KIP */
#define ARM_SYSCALL_PAGE		0xFFFFF000

extern unsigned int __syscall_page_start;

typedef struct syscall_args {
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3;		/* MR0 */
	u32 r4;		/* MR1 */
	u32 r5;		/* MR2 */
	u32 r6;		/* MR3 */
	u32 r7;		/* MR4 */
	u32 r8;		/* MR5 */
} syscall_args_t;

typedef struct msg_regs {
	u32 mr0;
	u32 mr1;
	u32 mr2;
	u32 mr3;
	u32 mr4;
	u32 mr5;
} msg_regs_t;

/* NOTE:
 * These references are valid only when they have been explicitly set
 * by a kernel entry point, e.g. a system call, a data abort handler.
 */
#define KTCB_REF_ARG0(ktcb)	(&(ktcb)->syscall_regs->r0)
#define KTCB_REF_MR0(ktcb)	(&(ktcb)->syscall_regs->r3)

/* Represents each syscall. We get argument registers
 * from stack for now. This is slower but the simplest. */
typedef int (*syscall_fn_t)(struct syscall_args *regs);

/* Entry point for syscall dispatching. Called from asm */
int syscall(struct syscall_args *regs, unsigned long);

/* Syscall-related initialiser called during system init. */
void syscall_init(void);
void kip_init_syscalls(void);

#endif /* __ARM_GLUE_SYSCALL_H__ */
