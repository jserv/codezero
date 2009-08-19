/*
 * Capability checking for all system calls
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/generic/resource.h>
#include <l4/generic/capability.h>

void capability_init(struct capability *cap)
{
	cap->capid = id_new(&kernel_container.capability_ids);
	link_init(&cap->list);
}

struct capability *capability_create(void)
{
	struct capability *cap = alloc_capability();

	capability_init(cap);

	return cap;
}

