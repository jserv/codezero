/*
 * Codezero Capability Definitions
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __CAPABILITY_H__
#define __CAPABILITY_H__

#include <l4/lib/list.h>

/*
 * A capability is a unique representation of security
 * qualifiers on a particular resource.
 *
 * In this structure:
 *
 * The capid denotes the unique capability ID. The resid denotes the unique ID
 * of targeted resource. The owner denotes the unique ID of capability owner.
 * This is almost always a thread ID.
 *
 * The type field contains two types: The capability type, and the targeted
 * resource type. The targeted resouce type denotes what type of resource the
 * capability is allowed to operate on. For example a thread, a thread group,
 * an address space or a memory can be of this type.
 *
 * The capability type defines the general set of operations allowed on a
 * particular resource. The resource type defines the type of resource that
 * the capability is targeting. For example a capability type may be
 * thread_control, exchange_registers, ipc, or map operations. A resource type
 * may be such as a thread, a thread group, a virtual or physical memory
 * region.
 *
 * There are also quantitative capability types. While their names denote
 * quantitative objects such as memory, threads, and address spaces, these
 * types actually define the quantitative operations available on those
 * resources such as creation and deletion of a thread, allocation and
 * deallocation of a memory region etc.
 *
 * The access field denotes the fine-grain operations available on a particular
 * resource. The meaning of each bitfield differs according to the type of the
 * capability. For example, for a capability type thread_control, the bitfields
 * may mean suspend, resume, create, delete etc.
 */
struct capability {
	struct link list;

	/* Capability identifiers */
	l4id_t capid;		/* Unique capability ID */
	l4id_t resid;		/* Targeted resource ID */
	l4id_t owner;		/* Capability owner ID */
	unsigned int type;	/* Capability and target resource type */

	/* Capability limits/permissions */
	u32 access;		/* Permitted operations */

	/* Limits on the resource */
	unsigned long start;	/* Resource start value */
	unsigned long end;	/* Resource end value */
	unsigned long size;	/* Resource size */
};

struct cap_list {
	int ncaps;
	struct link caps;
};

#if 0
/* Virtual memory space allocated to container */
struct capability cap_virtmap = {
	.capid = id_alloc(capids),
	.resid = container_id,
	.owner = pagerid,
	.type = CAP_TYPE_VIRTMEM,
	.access = 0, /* No access operations */
	.start = 0xF0000000,
	.end = 0xF1000000,
	.size = 0x1000000
};

/* Physical memory space allocated to container */
struct capability cap_physmap = {
	.capid = id_alloc(capids),
	.resid = container_id,
	.owner = pagerid,
	.type = CAP_TYPE_PHYSMEM,
	.access = 0, /* No access operations */
	.start = 0x0,
	.end = 0x1000000,
	.size = 0x1000000
};

/* IPC operations permitted on target thread */
struct capability cap_ipc = {
	.capid = id_alloc(capids),
	.resid = target_tid,
	.owner = tid,
	.type = CAP_TYPE_IPC,
	.access = CAP_IPC_SEND | CAP_IPC_RECV | CAP_IPC_FULL | CAP_IPC_SHORT | CAP_IPC_EXTENDED,
	.start = 0xF0000000,
	.end = 0xF1000000,
	.size = 0x1000000
};

/* Thread control operations permitted on target thread */
struct capability cap_thread_control = {
	.capid = id_alloc(capids),
	.resid = target_tid,
	.owner = pagerid,
	.type = CAP_TYPE_THREAD_CONTROL,
	.access = CAP_THREAD_SUSPEND | CAP_THREAD_RUN | CAP_THREAD_RECYCLE | CAP_THREAD_CREATE | CAP_THREAD_DESTROY,
	.start = 0,
	.end = 0,
	.size = 0,
};

/* Exregs operations permitted on target thread */
struct capability cap_exregs = {
	.capid = id_alloc(capids),
	.resid = target_tid,
	.owner = pagerid,
	.type = CAP_TYPE_EXREGS,
	.access = CAP_EXREGS_RW_PAGER | CAP_EXREGS_RW_SP | CAP_EXREGS_RW_PC | CAP_EXREGS_RW_UTCB | CAP_EXREGS_RW_OTHERS,
	.start = 0,
	.end = 0,
	.size = 0
};

/* Number of threads allocated to container */
struct capability cap_threads = {
	.capid = id_alloc(capids),
	.resid = container_id,
	.owner = pagerid,
	.type = CAP_TYPE_THREADS,
	.access = 0,
	.start = 0,
	.end = 0,
	.size = 256,
};

/* Number of spaces allocated to container */
struct capability cap_spaces = {
	.capid = id_alloc(capids),
	.resid = container_id,
	.owner = pagerid,
	.type = CAP_TYPE_SPACES,
	.access = 0,
	.start = 0,
	.end = 0,
	.size = 128,
};

/* CPU time allocated to container */
struct capability cap_cputime = {
	.capid = id_alloc(capids),
	.resid = container_id,
	.owner = pagerid,
	.type = CAP_TYPE_CPUTIME,
	.access = 0,
	.start = 0,
	.end = 0,
	.size = 55,	/* Percentage */
};

struct capability cap_cpuprio = {
	.capid = id_alloc(capids),
	.resid = container_id,
	.owner = pagerid,
	.type = CAP_TYPE_CPUPRIO,
	.access = 0,
	.start = 0,
	.end = 0,
	.size = 55,	/* Priority No */
};

#endif

#endif /* __CAPABILITY_H__ */
