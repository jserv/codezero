/*
 * Syscall offsets in the syscall page.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include INC_GLUE(syscall.h)

#define syscall_offset_mask			0xFF

#define	sys_ipc_offset				0x0
#define sys_thread_switch_offset		0x4
#define sys_thread_control_offset		0x8
#define sys_exchange_registers_offset		0xC
#define sys_schedule_offset			0x10
#define sys_unmap_offset			0x14
#define sys_space_control_offset		0x18
#define sys_ipc_control_offset			0x1C
#define sys_map_offset				0x20
#define sys_getid_offset			0x24
#define sys_kread_offset			0x28
#define sys_kmem_control_offset			0x2C
#define sys_time_offset				0x30
#define syscalls_end_offset			sys_time_offset
#define SYSCALLS_TOTAL				((syscalls_end_offset >> 2) + 1)

int sys_ipc(struct syscall_args *);
int sys_thread_switch(struct syscall_args *);
int sys_thread_control(struct syscall_args *);
int sys_exchange_registers(struct syscall_args *);
int sys_schedule(struct syscall_args *);
int sys_unmap(struct syscall_args *);
int sys_space_control(struct syscall_args *);
int sys_ipc_control(struct syscall_args *);
int sys_map(struct syscall_args *);
int sys_getid(struct syscall_args *);
int sys_kread(struct syscall_args *);
int sys_kmem_control(struct syscall_args *);
int sys_time(struct syscall_args *);

#endif /* __SYSCALL_H__ */
