/*
 * Initialise system call offsets.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4lib/kip.h>
#include <l4lib/arch/syslib.h>
#include <l4/macros.h>
#include INC_GLUE(memlayout.h)

__l4_ipc_t __l4_ipc = 0;
__l4_map_t __l4_map = 0;
__l4_unmap_t __l4_unmap = 0;
__l4_kread_t __l4_kread = 0;
__l4_getid_t __l4_getid = 0;
__l4_thread_switch_t __l4_thread_switch = 0;
__l4_thread_control_t __l4_thread_control = 0;
__l4_ipc_control_t __l4_ipc_control = 0;
__l4_space_control_t __l4_space_control = 0;
__l4_exchange_registers_t __l4_exchange_registers = 0;
__l4_kmem_grant_t __l4_kmem_grant = 0;
__l4_kmem_reclaim_t __l4_kmem_reclaim = 0;

struct kip *kip;

/* UTCB address of this task. */
struct utcb *utcb;
#include <stdio.h>

void __l4_init(void)
{
	kip = l4_kernel_interface(0, 0, 0);

	__l4_ipc =		(__l4_ipc_t)kip->ipc;
	__l4_map =		(__l4_map_t)kip->map;
	__l4_unmap =		(__l4_unmap_t)kip->unmap;
	__l4_kread =		(__l4_kread_t)kip->kread;
	__l4_getid =		(__l4_getid_t)kip->getid;
	__l4_thread_switch =	(__l4_thread_switch_t)kip->thread_switch;
	__l4_thread_control=	(__l4_thread_control_t)kip->thread_control;
	__l4_ipc_control=	(__l4_ipc_control_t)kip->ipc_control;
	__l4_space_control=	(__l4_space_control_t)kip->space_control;
	__l4_exchange_registers =
			(__l4_exchange_registers_t)kip->exchange_registers;
	__l4_kmem_grant =	(__l4_kmem_grant_t)kip->kmem_grant;
	__l4_kmem_reclaim =	(__l4_kmem_reclaim_t)kip->kmem_reclaim;

	/* Initialise utcb only if we're not the pager */
	if (self_tid() != PAGER_TID) {
		/* FIXME: C library should give the environ pointer for this */
		utcb = *(struct utcb **)(USER_AREA_END - PAGE_SIZE);
		printf("UTCB Read from userspace as: 0x%x\n",
		(unsigned long)utcb);
	}
}

