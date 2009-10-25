/*
 * Codezero Capability Definitions
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __GENERIC_CAPABILITY_H__
#define __GENERIC_CAPABILITY_H__

#include <l4/lib/list.h>
#include <l4/api/exregs.h>

/*
 * Some resources that capabilities possess don't
 * have unique ids or need ids at all.
 *
 * E.g. a threadpool does not need a resource id.
 * A virtual memory capability does not require
 * a resource id, its capid is sufficient.
 */
#define CAP_RESID_NONE		-1

/*
 * A capability is a unique representation of security
 * qualifiers on a particular resource.
 *
 * In this structure:
 *
 * The capid denotes the unique capability ID.
 * The resid denotes the unique ID of targeted resource.
 * The owner denotes the unique ID of the one and only capability owner. This is
 * almost always a thread ID.
 *
 * The type field contains two types:
 * 	- The capability type,
 * 	- The targeted resource type.
 *
 * The targeted resouce type denotes what type of resource the capability is
 * allowed to operate on. For example a thread, a thread group, an address space
 * or a memory can be of this type.
 *
 * The capability type defines the general set of operations allowed on a
 * particular resource. For example a capability type may be thread_control,
 * exchange_registers, ipc, or map operations. A resource type may be such as a
 * thread, a thread group, a virtual or physical memory region.
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

	/* Used amount on resource */
	unsigned long used;
};

struct cap_list {
	int ktcb_refs;
	int ncaps;
	struct link caps;
};

void capability_init(struct capability *cap);
struct capability *capability_create(void);
struct capability *boot_capability_create(void);


static inline void cap_list_init(struct cap_list *clist)
{
	clist->ncaps = 0;
	link_init(&clist->caps);
}

static inline void cap_list_insert(struct capability *cap,
				   struct cap_list *clist)
{
	list_insert(&cap->list, &clist->caps);
	clist->ncaps++;
}

/* Detach a whole list of capabilities from list head */
static inline struct capability *
cap_list_detach(struct cap_list *clist)
{
	struct link *list = list_detach(&clist->caps);
	clist->ncaps = 0;
	return link_to_struct(list, struct capability, list);
}

/* Attach a whole list of capabilities to list head */
static inline void cap_list_attach(struct capability *cap,
				   struct cap_list *clist)
{
	/* Attach as if cap is the list and clist is the element */
	list_insert(&clist->caps, &cap->list);

	/* Count the number of caps attached */
	list_foreach_struct(cap, &clist->caps, list)
		clist->ncaps++;
}

static inline void cap_list_move(struct cap_list *to,
				 struct cap_list *from)
{
	struct capability *cap_head = cap_list_detach(from);
	cap_list_attach(cap_head, to);
}

/* Have to have these as tcb.h includes this file */
struct ktcb;
struct task_ids;

/* Capability checking for quantitative capabilities */
int capability_consume(struct capability *cap, int quantity);
int capability_free(struct capability *cap, int quantity);
struct capability *capability_find_by_rtype(struct ktcb *task,
					    unsigned int rtype);

struct capability *cap_list_find_by_rtype(struct cap_list *clist,
					  unsigned int rtype);

/* Capability checking on system calls */
int cap_map_check(struct ktcb *task, unsigned long phys, unsigned long virt,
		  unsigned long npages, unsigned int flags, l4id_t tid);
int cap_thread_check(struct ktcb *task, unsigned int flags,
		     struct task_ids *ids);
int cap_exregs_check(struct ktcb *task, struct exregs_data *exregs);
int cap_ipc_check(l4id_t to, l4id_t from,
		  unsigned int flags, unsigned int ipc_type);
int cap_cap_check(struct ktcb *task, unsigned int req, unsigned int flags);
int cap_mutex_check(unsigned long mutex_address, int mutex_op);

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

#endif /* __GENERIC_CAPABILITY_H__ */
