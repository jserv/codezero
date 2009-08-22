/*
 * Capability checking for all system calls
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/generic/resource.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/api/errno.h>
#include <l4/lib/printk.h>

void capability_init(struct capability *cap)
{
	cap->capid = id_new(&kernel_container.capability_ids);
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

/*
 * Find a capability from a list by its resource type
 */
struct capability *capability_find_by_rtype(struct cap_list *clist,
					    unsigned int rtype)
{
	struct capability *cap;

	list_foreach_struct(cap, &clist->caps, list) {
		if ((cap->type & CAP_RTYPE_MASK) == rtype)
			return cap;
	}
	return 0;
}

