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

	/* Search task's own list */
	list_foreach_struct(cap, &task->cap_list.caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

	/* Search space list */
	list_foreach_struct(cap, &task->space->cap_list.caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

	/* Search thread group list */
	list_foreach_struct(cap, &task->tgr_cap_list.caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

	/* Search container list */
	list_foreach_struct(cap, &task->container->cap_list.caps, list)
		if (cap_rtype(cap) == rtype)
			return cap;

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

