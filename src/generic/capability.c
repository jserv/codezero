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
#include <l4/api/errno.h>
#include <l4/lib/printk.h>

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
 * Tasks won't always search for a capability randomly. Consider
 * mutexes, if a mutex is freed, it needs to be accounted to private
 * pool first if that is not full, because freeing it into shared
 * pool may lose the mutex right to another task.
 */
struct capability *capability_find_by_rtype(struct ktcb *task,
					    unsigned int rtype)
{
	struct capability *cap;
	struct ktcb *tgleader, *pager;

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

typedef struct capability *(cap_match_func *)
	(struct capability *cap, void *match_args)
	cap_match_func_t;

struct capability *cap_find(struct ktcb *task, cap_match_func_t cap_match_func,
			    void *match_args, unsigned int val)
{
	struct capability *cap;
	struct ktcb *tgleader, *pager;

	/* Search task's own list */
	list_foreach_struct(cap, &task->cap_list.caps, list)
		if ((cap = cap_match_func(cap, match_args, val)))
			return cap;

	/* Search space list */
	list_foreach_struct(cap, &task->space->cap_list.caps, list)
		if ((cap = cap_match_func(cap, match_args, val)))
			return cap;

	/* Search container list */
	list_foreach_struct(cap, &task->container->cap_list.caps, list)
		if ((cap = cap_match_func(cap, match_args, val)))
			return cap;

	return 0;
}


struct capability *
cap_match_mem(struct capability *cap,
	      void *match_args, unsigned int valid)
{
	struct capability *match = match_args;

	/* Equality-check these fields based on valid vector */
	if (valid & FIELD_TO_BIT(struct capability, capid))
		if (cap->capid != match->capid)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, resid))
		if (cap->resid != match->resid)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, owner))
		if (cap->owner != match->owner)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, type))
		if (cap->type != match->type)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, access))
		if ((cap->access & match->access)
		    != match->access)
			return 0;

	/* Checked these together as a range */
	if (valid & FIELD_TO_BIT(struct capability, start) ||
	    valid & FIELD_TO_BIT(struct capability, end))
		if (!(match->start >= cap->start &&
		      match->end <= cap->end &&
		      match->start < match->end))
			return 0;

	/* It is a match */
	return cap;
}

struct ipc_match = {
	l4id_t tid;
	l4id_t tgid;
	l4id_t spid;
	l4id_t cid;
	struct capability *cap;
};

/*
 * In an ipc, we could look for access bits, resource type and target id
 */
struct capability *
cap_match_ipc(struct capability *cap, void *match_args, unsigned int valid)
{
	struct ipc_match *ipc_match = match_args;
	struct capability *cap = ipc_match->cap;

	/*
	 * Check these for basic equality.
	 */
	if (valid & FIELD_TO_BIT(struct capability, capid))
		if (cap->capid != match->capid)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, owner))
		if (cap->owner != match->owner)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, access))
		if ((cap->access & match->access)
		    != match->access)
			return 0;

	/*
	 * Check these optimised/specially.
	 * rtype and target are checked against each
	 * other all at once
	 */
	if (valid & FIELD_TO_BIT(struct capability, resid)) {
		switch (cap_rtype(cap)) {
		case CAP_RTYPE_THREAD:
			if (ipc_matcher->tid != resid)
				return 0;
			break;
		case CAP_RTYPE_TGROUP:
			if (ipc_matcher->tgid != resid)
				return 0;
			break;
		case CAP_RTYPE_SPACE:
			if (ipc_matcher->spid != resid)
				return 0;
			break;
		case CAP_RTYPE_CONTAINER:
			if (ipc_matcher->cid != resid)
				return 0;
			break;
		/*
		 * It's simply a bug to
		 * get an unknown resource here
		 */
		default:
			BUG();
		}
	}
	return cap;
}

int cap_ipc_check(struct ktcb *target, l4id_t from,
		  unsigned int flags, unsigned int ipc_type)
{
	unsigned int valid = 0;
	struct capability ipccap = {
		.access = ipc_flags_type_to_access(flags, ipc_type);
	};

	/*
	 * All these ids will get checked at once,
	 * depending on the encountered capability's
	 * rtype field
	 */
	struct ipc_matcher ipc_matcher = {
		.tid = target->tid,
		.tgid = target->tgid,
		.spid = target->space->spid,
		.cid = target->container->cid,
		.ipccap = ipccap,
	};

	valid |= FIELD_TO_BIT(struct capability, access);
	valid |= FIELD_TO_BIT(struct capability, resid);

	if (!(cap_find(task, cap_match_ipc,
		       &ipc_matcher, valid)))
		return -ENOCAP;
	return 0;
}

struct exregs_match = {
	l4id_t tid
	l4id_t pagerid;
	l4id_t tgid;
	l4id_t spid;
	l4id_t cid;
	struct capability *cap;
};


struct capability *
cap_match_thread(struct capability *cap, void *match_args, unsigned int valid)
{
	struct thread_match *match = match_args;
	struct capability *cap = match->cap;

	/*
	 * Check these for basic equality.
	 */
	if (valid & FIELD_TO_BIT(struct capability, capid))
		if (cap->capid != match->capid)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, owner))
		if (cap->owner != match->owner)
			return 0;
	if (valid & FIELD_TO_BIT(struct capability, access))
		if ((cap->access & match->access)
		    != match->access)
			return 0;

	/*
	 * Check these optimised/specially.
	 * rtype and target are checked against each
	 * other all at once
	 */
	if (valid & FIELD_TO_BIT(struct capability, resid)) {
		switch (cap_rtype(cap)) {
		/* Ability to thread_control over a paged group */
		case CAP_RTYPE_PGGROUP:
			if (match->pagerid != resid)
				return 0;
			break;
		case CAP_RTYPE_TGROUP:
			if (match->tgid != resid)
				return 0;
			break;
		case CAP_RTYPE_SPACE:
			if (match->spid != resid)
				return 0;
			break;
		case CAP_RTYPE_CONTAINER:
			if (match->cid != resid)
				return 0;
			break;
		/*
		 * It's simply a bug to
		 * get an unknown resource here
		 */
		default:
			BUG();
		}
	}
	return cap;
}

/*
 * TODO: We are here!!!
 */
int cap_exregs_check(unsigned int flags, struct task_ids *ids)
{
	struct capability cap = {
		.access = thread_control_flags_to_access(flags);
		/* all resid's checked all at once by comparing against rtype */
	};
	struct thread_match match = {
		.pagerid = current->tid
		.tgid = current->tgid,
		.spid = current->spid,
		.cid = current->cid,
		.cap = threadmatch,
	};
	unsigned int valid = 0;

	valid |= FIELD_TO_BIT(struct capability, access);
	valid |= FIELD_TO_BIT(struct capability, resid);

	if (!(cap_find(task, cap_match_thread,
		       &thread_match, valid)))
		return -ENOCAP;
	return 0;
}

/*
 * FIXME: As new pagers, thread groups,
 * thread ids, spaces are created, we need to
 * give them thread_control capabilities dynamically,
 * based on those ids!!! How do we get to do that, so that
 * in userspace it looks not so difficult ???
 */
struct thread_match = {
	l4id_t tgid;
	l4id_t tid;
	l4id_t pagerid;
	l4id_t spid;
	l4id_t cid;
	unsigned int thread_control_flags;
};

def thread_create():
	new_space, same_tgid, same_pager,
	check_existing_ids(tid, same_tgid, same_pager, thread_create)
	thread_create(new_space)
	thread_add_cap(new_space, all other caps)
/*
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
itn cap_thread_check(struct ktcb *task, unsigned int flags, struct task_ids *ids)
{
	struct thread_matcher = {
		.pagerid = current->tid
		.tgid = current->tgid,
		.spid = current->spid,
		.cid = current->cid,
		.thread_control_flags = flags;
	};
	unsigned int valid = 0;

	valid |= FIELD_TO_BIT(struct capability, access);
	valid |= FIELD_TO_BIT(struct capability, resid);

	if (!(cap_find(task, cap_match_thread,
		       &thread_matcher, valid)))
		return -ENOCAP;
	return 0;
}

int cap_map_check(struct ktcb *task, unsigned long phys, unsigned long virt,
		  unsigned long npages, unsigned int flags, l4id_t tid)
{
	struct capability *physmem, *virtmem;
	struct capability physmatch = {
		.start = __pfn(phys),
		.end = __pfn(phys) + npages,
		.type = CAP_TYPE_PHYSMEM,
		.flags = map_flags_to_cap_flags(flags),
	}
	struct capability virtmatch = {
		.start = __pfn(virt),
		.end = __pfn(virt) + npages,
		.type = CAP_TYPE_VIRTMEM,
		.flags = map_flags_to_cap_flags(flags),
	}
	unsigned int virt_valid = 0;
	unsigned int phys_valid = 0;

	virt_valid |= FIELD_TO_BIT(struct capability, access);
	virt_valid |= FIELD_TO_BIT(struct capability, start);
	virt_valid |= FIELD_TO_BIT(struct capability, end);

	phys_valid |= FIELD_TO_BIT(struct capability, access);
	phys_valid |= FIELD_TO_BIT(struct capability, start);
	phys_valid |= FIELD_TO_BIT(struct capability, end);

	if (!(physmem =	cap_find(task, cap_match_mem,
				 &physmatch, phys_valid)))
		return -ENOCAP;

	if (!(virtmem = cap_find(task, cap_match_mem,
				 &virtmatch, virt_valid)))
		return -ENOCAP;

	return 0;
}

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
	case CAP_RTYPE_UMUTEX:
		break;
	}
	return 0;
}

