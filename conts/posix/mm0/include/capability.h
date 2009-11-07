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


extern struct cap_list capability_list;

struct initdata;
int read_pager_capabilities();
void copy_boot_capabilities();

#endif /* __MM0_CAPABILITY_H__ */
