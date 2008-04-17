/*
 * System call prototypes.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __ARM_SYSCALLS_H__
#define __ARM_SYSCALLS_H__

#include <l4lib/arch/types.h>
#include <l4lib/arch/utcb.h>
#include <l4/generic/space.h>
#include <l4/api/space.h>
#include <l4/api/kip.h>
#include <l4/api/ipc.h>
#include <l4/api/thread.h>

static inline void *
l4_kernel_interface(unsigned int *api_version, unsigned int *api_flags,
		   unsigned int *kernel_id)
{
	return (void *)L4_KIP_ADDRESS;
}

typedef unsigned int (*__l4_thread_switch_t)(u32);
extern __l4_thread_switch_t __l4_thread_switch;
unsigned int l4_thread_switch (u32 dest);

typedef int (*__l4_getid_t)(struct task_ids *ids);
extern __l4_getid_t __l4_getid;
int l4_getid(struct task_ids *ids);

typedef int (*__l4_ipc_t)(l4id_t to, l4id_t from);
extern __l4_ipc_t __l4_ipc;
int l4_ipc(l4id_t to, l4id_t from);

typedef int (*__l4_kread_t)(u32 rd, void *addr);
extern __l4_kread_t __l4_kread;
int l4_kread(u32 rd, void *addr);

typedef int (*__l4_map_t)(void *phys, void *virt,
			  u32 npages, u32 flags, l4id_t tid);
extern __l4_map_t __l4_map;
int l4_map(void *p, void *v, u32 npages, u32 flags, l4id_t tid);

typedef int (*__l4_unmap_t)(void *virt, unsigned long npages, l4id_t tid);
extern __l4_unmap_t __l4_unmap;
int l4_unmap(void *virtual, unsigned long numpages, l4id_t tid);

typedef int (*__l4_thread_control_t)(unsigned int action, struct task_ids *ids);
extern __l4_thread_control_t __l4_thread_control;
int l4_thread_control(unsigned int action, struct task_ids *ids);

typedef int (*__l4_space_control_t)(unsigned int action, void *kdata);
extern __l4_space_control_t __l4_space_control;
int l4_space_control(unsigned int action, void *kdata);

typedef int (*__l4_ipc_control_t)(unsigned int action, l4id_t blocked_sender,
				  u32 blocked_tag);
extern __l4_ipc_control_t __l4_ipc_control;
int l4_ipc_control(unsigned int, l4id_t blocked_sender, u32 blocked_tag);

typedef int (*__l4_exchange_registers_t)(unsigned int pc, unsigned int sp,
					 l4id_t pager, l4id_t tid);
extern __l4_exchange_registers_t __l4_exchange_registers;
int l4_exchange_registers(unsigned int pc, unsigned int sp, int pager, l4id_t tid);

typedef int (*__l4_kmem_control_t)(unsigned long pfn, int npages, int grant);
extern __l4_kmem_control_t __l4_kmem_control;
int l4_kmem_control(unsigned long pfn, int npages, int grant);

typedef int (*__l4_time_t)(void *time_info, int set);
extern __l4_time_t __l4_time;
int l4_time(void *time_info, int set);



/* To be supplied by server tasks. */
void *virt_to_phys(void *);
void *phys_to_virt(void *);


#endif /* __ARM_SYSCALLS_H__ */

