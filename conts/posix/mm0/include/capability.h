/*
 * Capability-related operations of the pager.
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __MM0_CAPABILITY_H__
#define __MM0_CAPABILITY_H__

#include <l4lib/types.h>
#include <l4/lib/list.h>

struct cap_list {
	int ncaps;
	struct link caps;
};

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

	unsigned long used;	/* Resource used size */
};


extern struct cap_list capability_list;

struct initdata;
int read_kernel_capabilities(struct initdata *);
void copy_boot_capabilities(struct initdata *initdata);

#endif /* __MM0_CAPABILITY_H__ */
