/*
 * Capability checking for all system calls
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/generic/resource.h>
#include <l4/generic/capability.h>
#include <l4/generic/container.h>
#include <l4/generic/cap-types.h>
#include <l4/generic/tcb.h>
#include <l4/api/capability.h>
#include <l4/api/thread.h>
#include <l4/api/errno.h>
#include <l4/lib/printk.h>
#include <l4/api/thread.h>
#include <l4/api/exregs.h>
#include <l4/api/ipc.h>
#include INC_GLUE(message.h)

void capability_init(struct capability *cap)
{
	cap->capid = id_new(&kernel_resources.capability_ids);
	link_init(&cap->list);
}

/*
 * Boot-time function to create capability without
 * capability checking
 */
struct capability *boot_capability_create(void)
{
	struct capability *cap = boot_alloc_capability();

	capability_init(cap);

	return cap;
}

struct capability *capability_create(void)
{
	struct capability *cap = alloc_capability();

	capability_init(cap);

	return cap;
}

#if defined(CONFIG_CAPABILITIES)
int capability_consume(struct capability *cap, int quantity)
{
	if (cap->size < cap->used + quantity)
		return -ENOCAP;

	cap->used += quantity;

	return 0;
}

int capability_free(struct capability *cap, int quantity)
{
	BUG_ON((cap->used -= quantity) < 0);
	return 0;
}

#else
int capability_consume(struct capability *cap, int quantity)
{
	return 0;
}

int capability_free(struct capability *cap, int quantity)
{
	return 0;
}
#endif

struct capability *cap_list_find_by_rtype(struct cap_list *cap_list,
					  unsigned int rtype)
{
	struct capability *cap;

	list_foreach_struct(cap, &cap_list->caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

	return 0;
}

/*
 * Find a capability from a list by its resource type
 * Search all capability lists that task is allowed.
 *
 * FIXME:
 * Tasks should not always search for a capability randomly. Consider
 * mutexes, if a mutex is freed, it needs to be accounted to private
 * pool first if that is not full, because freeing it into shared
 * pool may lose the mutex right to another task. In other words,
 * when you're freeing a mutex, we should know which capability pool
 * to free it to.
 *
 * In conclusion freeing of pool-type capabilities need to be done
 * in order of privacy. -> It may get confusing as a space, thread
 * group id or paged-thread group is not necessarily in a different
 * privacy ring.
 */
struct capability *capability_find_by_rtype(struct ktcb *task,
					    unsigned int rtype)
{
	struct capability *cap;

	/* Search task's own list */
	list_foreach_struct(cap, &task->cap_list.caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

	/* Search space list */
	list_foreach_struct(cap, &task->space->cap_list.caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

	/* Search container list */
	list_foreach_struct(cap, &task->container->cap_list.caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

	return 0;
}

typedef struct capability *(*cap_match_func_t) \
		(struct capability *cap, void *match_args);

struct capability *cap_find(struct ktcb *task, cap_match_func_t cap_match_func,
			     void *match_args, unsigned int cap_type)
{
	struct capability *cap, *found;

	/* Search task's own list */
	list_foreach_struct(cap, &task->cap_list.caps, list)
		if (cap_type(cap) == cap_type &&
		    ((found = cap_match_func(cap, match_args))))
			return found;

	/* Search space list */
	list_foreach_struct(cap, &task->space->cap_list.caps, list)
		if (cap_type(cap) == cap_type &&
		    ((found = cap_match_func(cap, match_args))))
			return found;

	/* Search container list */
	list_foreach_struct(cap, &task->container->cap_list.caps, list)
		if (cap_type(cap) == cap_type &&
		    ((found = cap_match_func(cap, match_args))))
			return found;

	return 0;
}

struct sys_mutex_args {
	unsigned long address;
	unsigned int op;
};

/*
 * Check broadly the ability to do mutex ops. Check it by
 * the thread, space or container, (i.e. the group that can
 * do this operation broadly)
 *
 * Note, that we check mutex_address elsewhere as a quick,
 * per-task virt_to_phys translation that would not get
 * easily/quickly satisfied by a memory capability checking.
 *
 * While this is not %100 right from a capability checking
 * point-of-view, it is a shortcut that works and makes sense.
 *
 * For sake of completion, the right way to do it would be to
 * add MUTEX_LOCKABLE, MUTEX_UNLOCKABLE attributes to both
 * virtual and physical memory caps of a task, search those
 * to validate the address. But we would have to translate
 * from the page tables either ways.
 */
struct capability *
cap_match_mutex(struct capability *cap, void *args)
{
	/* Unconditionally expect these flags */
	unsigned int perms = CAP_UMUTEX_LOCK | CAP_UMUTEX_UNLOCK;

	if ((cap->access & perms) != perms)
		return 0;

	/* Now check the usual restype/resid pair */
	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
		if (current->tid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_SPACE:
		if (current->space->spid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_CONTAINER:
		if (current->container->cid != cap->resid)
			return 0;
		break;
	default:
		BUG(); /* Unknown cap type is a bug */
	}

	return cap;
}

struct sys_capctrl_args {
	unsigned int req;
	unsigned int flags;
	struct ktcb *task;
};

struct capability *
cap_match_capctrl(struct capability *cap, void *args_ptr)
{
	struct sys_capctrl_args *args = args_ptr;
	unsigned int req = args->req;
	struct ktcb *target = args->task;

	/* Check operation privileges */
	if (req == CAP_CONTROL_NCAPS ||
	    req == CAP_CONTROL_READ)
		if (!(cap->access & CAP_CAP_READ))
			return 0;
	if (req == CAP_CONTROL_SHARE)
		if (!(cap->access & CAP_CAP_SHARE))
			return 0;
	if (req == CAP_CONTROL_GRANT)
		if (!(cap->access & CAP_CAP_GRANT))
			return 0;
	if (req == CAP_CONTROL_MODIFY)
		if (!(cap->access & CAP_CAP_MODIFY))
			return 0;

	/* Now check the usual restype/resid pair */
	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
		if (target->tid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_SPACE:
		if (target->space->spid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_CONTAINER:
		if (target->container->cid != cap->resid)
			return 0;
		break;
	default:
		BUG(); /* Unknown cap type is a bug */
	}

	return cap;
}

struct sys_ipc_args {
	struct ktcb *task;
	unsigned int ipc_type;
	unsigned int flags;
};

/*
 * In an ipc, we could look for access bits, resource type and target id
 */
struct capability *
cap_match_ipc(struct capability *cap, void *args_ptr)
{
	struct sys_ipc_args *args = args_ptr;
	struct ktcb *target = args->task;

	/* Check operation privileges */
	if (args->flags & IPC_FLAGS_SHORT)
		if (!(cap->access & CAP_IPC_SHORT))
			return 0;
	if (args->flags & IPC_FLAGS_FULL)
		if (!(cap->access & CAP_IPC_FULL))
			return 0;
	if (args->flags & IPC_FLAGS_EXTENDED)
		if (!(cap->access & CAP_IPC_EXTENDED))
			return 0;

	/* Assume we have both send and receive unconditionally */
	if (!((cap->access & CAP_IPC_SEND) &&
	      (cap->access & CAP_IPC_RECV)))
		return 0;

	/*
	 * We have a target thread, check if capability match
	 * any resource fields in target
	 */
	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
		if (target->tid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_SPACE:
		if (target->space->spid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_CONTAINER:
		if (target->container->cid != cap->resid)
			return 0;
		break;
	default:
		BUG(); /* Unknown cap type is a bug */
	}

	return cap;
}

struct sys_exregs_args {
	struct exregs_data *exregs;
	struct ktcb *task;
};

/*
 * CAP_TYPE_EXREGS already matched upon entry
 */
struct capability *
cap_match_exregs(struct capability *cap, void *args_ptr)
{
	struct sys_exregs_args *args = args_ptr;
	struct exregs_data *exregs = args->exregs;
	struct ktcb *target = args->task;

	/* Check operation privileges */
	if (exregs->valid_vect & EXREGS_VALID_REGULAR_REGS)
		if (!(cap->access & CAP_EXREGS_RW_REGS))
			return 0;
	if (exregs->valid_vect & EXREGS_VALID_SP)
		if (!(cap->access & CAP_EXREGS_RW_SP))
			return 0;
	if (exregs->valid_vect & EXREGS_VALID_PC)
		if (!(cap->access & CAP_EXREGS_RW_PC))
			return 0;
	if (args->exregs->valid_vect & EXREGS_SET_UTCB)
		if (!(cap->access & CAP_EXREGS_RW_UTCB))
			return 0;
	if (args->exregs->valid_vect & EXREGS_SET_PAGER)
		if (!(cap->access & CAP_EXREGS_RW_PAGER))
			return 0;

	/*
	 * We have a target thread, check if capability
	 * match any resource fields in target.
	 */
	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
		if (target->tid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_SPACE:
		if (target->space->spid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_CONTAINER:
		if (target->container->cid != cap->resid)
			return 0;
		break;
	default:
		BUG(); /* Unknown cap type is a bug */
	}

	return cap;
}

/*
 * FIXME: Issues on capabilities:
 *
 * As new pagers, thread groups,
 * thread ids, spaces are created, we need to
 * give them thread_control capabilities dynamically,
 * based on those ids!!! How do we get to do that, so that
 * in userspace it looks not so difficult ???
 *
 * What do you match here?
 *
 * THREAD_CREATE:
 *  - TC_SAME_SPACE
 *    - spid -> Does thread have cap to create in that space?
 *    - cid -> Does thread have cap to create in that container?
 *    - tgid -> Does thread have cap to create in that thread group?
 *    - pagerid -> Does thread have cap to create in that group of paged threads?
 *  - TC_NEW_SPACE or TC_COPY_SPACE
 *    - Check cid, tgid, pagerid,
 *  - TC_SHARE_GROUP
 *    - Check tgid
 *  - TC_AS_PAGER
 *    - pagerid -> Does thread have cap to create in that group of paged threads?
 *  - TC_SHARE_PAGER
 *    - pagerid -> Does thread have cap to create in that group of paged threads?
 *   New group -> New set of caps, thread_control, exregs, ipc, ... all of them!
 *   New pager -> New set of caps for that pager.
 *   New thread -> New set of caps for that thread!
 *   New space -> New set of caps for that space! So many capabilities!
 */

struct sys_tctrl_args {
	struct ktcb *task;
	unsigned int flags;
	struct task_ids *ids;
};

/*
 * CAP_TYPE_TCTRL matched upon entry
 */
struct capability *cap_match_thread(struct capability *cap,
				    void *args_ptr)
{
	struct sys_tctrl_args *args = args_ptr;
	struct ktcb *target = args->task;
	unsigned int action_flags = args->flags & THREAD_ACTION_MASK;

	/* Check operation privileges */
	switch (action_flags) {
	case THREAD_CREATE:
		if (!(cap->access & CAP_TCTRL_CREATE))
			return 0;
		break;
	case THREAD_DESTROY:
		if (!(cap->access & CAP_TCTRL_DESTROY))
			return 0;
		break;
	case THREAD_SUSPEND:
		if (!(cap->access & CAP_TCTRL_SUSPEND))
			return 0;
		break;
	case THREAD_RUN:
		if (!(cap->access & CAP_TCTRL_RUN))
			return 0;
		break;
	case THREAD_RECYCLE:
		if (!(cap->access & CAP_TCTRL_RECYCLE))
			return 0;
		break;
	default:
		/* We refuse to accept anything else */
		return 0;
	}

	/* If no target and create, or vice versa, it really is a bug */
	BUG_ON(!target && action_flags != THREAD_CREATE);
	BUG_ON(target && action_flags == THREAD_CREATE);

	if (action_flags == THREAD_CREATE) {
		/*
		 * FIXME: Add cid to task_ids arg.
		 *
		 * Its a thread create and we have no knowledge of
		 * thread id, space id, or any other id.
		 *
		 * We _assume_ target is the largest group,
		 * e.g. same container as current. We check
		 * for `container' as target in capability
		 */
		if (cap_rtype(cap) != CAP_RTYPE_CONTAINER)
			return 0;
		if (cap->resid != current->container->cid)
			return 0;
		/* Resource type and id match, success */
		return cap;
	}

	/*
	 * We have a target thread, check if capability match
	 * any resource fields in target
	 */
	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
		if (target->tid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_SPACE:
		if (target->space->spid != cap->resid)
			return 0;
		break;
	case CAP_RTYPE_CONTAINER:
		if (target->container->cid != cap->resid)
			return 0;
		break;
	default:
		BUG(); /* Unknown cap type is a bug */
	}
	return cap;
}

struct sys_map_args {
	struct ktcb *task;
	unsigned long phys;
	unsigned long virt;
	unsigned long npages;
	unsigned int flags;
	unsigned int rtype;
};

/*
 * CAP_TYPE_MAP already matched upon entry
 */
struct capability *cap_match_mem(struct capability *cap,
				 void *args_ptr)
{
	struct sys_map_args *args = args_ptr;
	unsigned long pfn;
	unsigned int perms;

	/* Set base according to what type of mem type we're matching */
	if (args->rtype == CAP_RTYPE_PHYSMEM)
		pfn = __pfn(args->phys);
	else
		pfn = __pfn(args->virt);

	/* Check range */
	if (cap->start > pfn || cap->end < pfn + args->npages)
		return 0;

	/* Check permissions */
	switch (args->flags) {
	case MAP_USR_RW_FLAGS:
		perms = CAP_MAP_READ | CAP_MAP_WRITE | CAP_MAP_CACHED;
		if ((cap->access & perms) != perms)
			return 0;
		break;
	case MAP_USR_RO_FLAGS:
		perms = CAP_MAP_READ | CAP_MAP_CACHED;
		if ((cap->access & perms) != perms)
			return 0;
		break;
	case MAP_USR_IO_FLAGS:
		perms = CAP_MAP_READ | CAP_MAP_WRITE | CAP_MAP_UNCACHED;
		if ((cap->access & perms) != perms)
			return 0;
		break;
	default:
		/* Anything else is an invalid/unrecognised argument */
		return 0;
	}

	return cap;

	/*
	 * FIXME:
	 *
	 * Does it make sense to have a meaningful resid field
	 * in a memory resource? E.g. Which resources may I map it to?
	 * It might, as I can map an arbitrary mapping to an arbitrary
	 * thread in my container and break it's memory integrity.
	 *
	 * It seems it would be reasonable for a pager to have memory
	 * capabilities with a resid of its own id, and rtype of
	 * CAP_RTYPE_PGGROUP, effectively allowing it to do map
	 * operations on itself and its group of paged children.
	 */
}

#if defined(CONFIG_CAPABILITIES)
int cap_mutex_check(unsigned long mutex_address, int mutex_op)
{
	struct sys_mutex_args args = {
		.address = mutex_address,
		.op = mutex_op,
	};

	if (!(cap_find(current, cap_match_mutex,
		       &args, CAP_TYPE_UMUTEX)))
		return -ENOCAP;

	return 0;
}

int cap_cap_check(struct ktcb *task, unsigned int req, unsigned int flags)
{
	struct sys_capctrl_args args = {
		.req = req,
		.flags = flags,
		.task = task,
	};

	if (!(cap_find(current, cap_match_capctrl,
		       &args, CAP_TYPE_CAP)))
		return -ENOCAP;

	return 0;
}

int cap_map_check(struct ktcb *target, unsigned long phys, unsigned long virt,
		  unsigned long npages, unsigned int flags)
{
	struct capability *physmem, *virtmem;
	struct sys_map_args args = {
		.task = target,
		.phys = phys,
		.virt = virt,
		.npages = npages,
		.flags = flags,
	};

	args.rtype = CAP_RTYPE_PHYSMEM;
	if (!(physmem =	cap_find(current, cap_match_mem,
				 &args, CAP_TYPE_MAP)))
		return -ENOCAP;

	args.rtype = CAP_RTYPE_VIRTMEM;
	if (!(virtmem = cap_find(current, cap_match_mem,
				 &args, CAP_TYPE_MAP)))
		return -ENOCAP;

	return 0;
}

/*
 * Limitation: We currently only check from sender's
 * perspective. Sender always targets a real thread.
 * Does sender have the right to do this ipc?
 */
int cap_ipc_check(l4id_t to, l4id_t from,
		  unsigned int flags, unsigned int ipc_type)
{
	struct ktcb *target;
	struct sys_ipc_args args;

	/* Receivers can get away from us (for now) */
	if (ipc_type != IPC_SEND  && ipc_type != IPC_SENDRECV)
		return 0;

	/*
	 * We're the sender, meaning we have
	 * a real target
	 */
	if (!(target = tcb_find(to)))
		return -ESRCH;

	/* Set up other args */
	args.flags = flags;
	args.ipc_type = ipc_type;
	args.task = target;

	if (!(cap_find(current, cap_match_ipc,
		       &args, CAP_TYPE_IPC)))
		return -ENOCAP;

	return 0;
}

int cap_exregs_check(struct ktcb *task, struct exregs_data *exregs)
{
	struct sys_exregs_args args = {
		.exregs = exregs,
		.task = task,
	};

	/* We always search for current's caps */
	if (!(cap_find(current, cap_match_exregs,
		       &args, CAP_TYPE_EXREGS)))
		return -ENOCAP;

	return 0;
}

int cap_thread_check(struct ktcb *task,
		     unsigned int flags,
		     struct task_ids *ids)
{
	struct sys_tctrl_args args = {
		.task = task,
		.flags = flags,
		.ids = ids,
	};

	if (!(cap_find(current, cap_match_thread,
		       &args, CAP_TYPE_TCTRL)))
		return -ENOCAP;

	return 0;
}

#else /* Meaning !CONFIG_CAPABILITIES */
int cap_mutex_check(unsigned long mutex_address, int mutex_op)
{
	return 0;
}

int cap_cap_check(struct ktcb *task, unsigned int req, unsigned int flags)
{
	return 0;
}

int cap_ipc_check(l4id_t to, l4id_t from,
		  unsigned int flags, unsigned int ipc_type)
{
	return 0;
}

int cap_map_check(struct ktcb *task, unsigned long phys, unsigned long virt,
		  unsigned long npages, unsigned int flags)
{
	return 0;
}

int cap_exregs_check(struct ktcb *task, struct exregs_data *exregs)
{
	return 0;
}

int cap_thread_check(struct ktcb *task,
		     unsigned int flags,
		     struct task_ids *ids)
{
	return 0;
}
#endif

/*
 * FIXME:
 *
 * Capability resource ids
 *
 * Currently the kernel cinfo has no resource id field. This is
 * because resources are dynamically assigned an id at run-time.
 *
 * However, this prevents the user to specify capabilities on
 * particular resources since resources aren't id-able at
 * configuration time. There's also no straightforward way to
 * determine which id was meant at run-time.
 *
 * As a solution, resources that are id-able and that are targeted
 * by capabilities at configuration time should be assigned static
 * ids at configuration time. Any other id-able resources that are
 * subject to capabilities by run-time sharing/delegation/derivation
 * etc. should be assigned their ids dynamically.
 *
 * Example:
 *
 * A capability to ipc to a particular container.
 *
 * When this capability is defined at configuration time, it is only
 * meaningful if the container id is also supplied statically.
 *
 * If a pager later on generates a capability to ipc to a thread
 * that it created at run-time, and grant it to another container's
 * pager, (say a container thread would like to talk to another
 * container's thread) it should assign the dynamically assigned
 * target thread id as the capability resource id and hand it over.
 */
int capability_set_resource_id(struct capability *cap)
{
	/* Identifiable resources */
	switch(cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
	case CAP_RTYPE_TGROUP:
	case CAP_RTYPE_SPACE:
	case CAP_RTYPE_CONTAINER:
		break;
	}
	return 0;
}

