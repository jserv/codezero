/*
 * System Calls
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include <l4/generic/space.h>
#include <l4/api/errno.h>
#include INC_GLUE(memlayout.h)
#include INC_GLUE(syscall.h)
#include INC_SUBARCH(mm.h)
#include INC_API(syscall.h)
#include INC_API(kip.h)

void kip_init_syscalls(void)
{
	kip.space_control = ARM_SYSCALL_PAGE + sys_space_control_offset;
	kip.thread_control = ARM_SYSCALL_PAGE + sys_thread_control_offset;
	kip.ipc_control = ARM_SYSCALL_PAGE + sys_ipc_control_offset;
	kip.map = ARM_SYSCALL_PAGE + sys_map_offset;
	kip.ipc = ARM_SYSCALL_PAGE + sys_ipc_offset;
	kip.kread = ARM_SYSCALL_PAGE + sys_kread_offset;
	kip.unmap = ARM_SYSCALL_PAGE + sys_unmap_offset;
	kip.exchange_registers = ARM_SYSCALL_PAGE + sys_exchange_registers_offset;
	kip.thread_switch = ARM_SYSCALL_PAGE + sys_thread_switch_offset;
	kip.schedule = ARM_SYSCALL_PAGE + sys_schedule_offset;
	kip.getid = ARM_SYSCALL_PAGE + sys_getid_offset;
	kip.kmem_grant = ARM_SYSCALL_PAGE + sys_kmem_grant_offset;
	kip.kmem_reclaim = ARM_SYSCALL_PAGE + sys_kmem_reclaim_offset;
}

/* Jump table for all system calls. */
syscall_fn_t syscall_table[SYSCALLS_TOTAL];

/*
 * Initialises the system call jump table, for kernel to use.
 * Also maps the system call page into userspace.
 */
void syscall_init()
{
	syscall_table[sys_ipc_offset >> 2] 			= (syscall_fn_t)sys_ipc;
	syscall_table[sys_thread_switch_offset >> 2] 		= (syscall_fn_t)sys_thread_switch;
	syscall_table[sys_thread_control_offset >> 2] 		= (syscall_fn_t)sys_thread_control;
	syscall_table[sys_exchange_registers_offset >> 2] 	= (syscall_fn_t)sys_exchange_registers;
	syscall_table[sys_schedule_offset >> 2] 		= (syscall_fn_t)sys_schedule;
	syscall_table[sys_getid_offset >> 2]	 		= (syscall_fn_t)sys_getid;
	syscall_table[sys_unmap_offset >> 2] 			= (syscall_fn_t)sys_unmap;
	syscall_table[sys_space_control_offset >> 2] 		= (syscall_fn_t)sys_space_control;
	syscall_table[sys_ipc_control_offset >> 2] 	= (syscall_fn_t)sys_ipc_control;
	syscall_table[sys_map_offset >> 2] 			= (syscall_fn_t)sys_map;
	syscall_table[sys_kread_offset >> 2]		 	= (syscall_fn_t)sys_kread;
	syscall_table[sys_kmem_grant_offset >> 2]		= (syscall_fn_t)sys_kmem_grant;
	syscall_table[sys_kmem_reclaim_offset >> 2]		= (syscall_fn_t)sys_kmem_reclaim;

	add_mapping(virt_to_phys(&__syscall_page_start),
		    ARM_SYSCALL_PAGE, PAGE_SIZE, MAP_USR_RO_FLAGS);
}

/* Checks a syscall is legitimate and dispatches to appropriate handler. */
int syscall(struct syscall_args *regs, unsigned long swi_addr)
{
	/* Check if genuine system call, coming from the syscall page */
	if ((swi_addr & ARM_SYSCALL_PAGE) == ARM_SYSCALL_PAGE) {
		/* Check within syscall offset boundary */
		if (((swi_addr & syscall_offset_mask) >= 0) &&
		    ((swi_addr & syscall_offset_mask) <= syscalls_end_offset)) {
			/* Quick jump, rather than compare each */
			return (*syscall_table[(swi_addr & 0xFF) >> 2])(regs);
		} else {
			printk("System call received from call @ 0x%lx."
			       "Instruction: 0x%lx.\n", swi_addr,
			       *((unsigned long *)swi_addr));
			return -ENOSYS;
		}
	} else {
		printk("System call exception from unknown location 0x%lx."
		       "Discarding.\n", swi_addr);
		return -ENOSYS;
	}
}

