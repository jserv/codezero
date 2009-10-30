/*
 * Capability manipulation syscall.
 *
 * The heart of Codezero security
 * mechanisms lay here.
 *
 * Copyright (C) 2009 Bahadir Balban
 */

#include <l4/api/capability.h>
#include <l4/generic/capability.h>
#include <l4/generic/container.h>
#include <l4/generic/tcb.h>
#include <l4/api/errno.h>
#include INC_API(syscall.h)

/*
 * FIXME: This is reading only a single list
 * there may be more than one
 */
int read_task_capabilities(void *userbuf)
{
	int copy_size, copy_offset = 0;
	struct capability *cap;
	int err;

	/*
	 * Currently only pagers can
	 * read their own capabilities
	 */
	if (current != current->pager->tcb)
		return -EPERM;

	/* Determine size of pager capabilities (FIXME: partial!) */
	copy_size = TASK_CAP_LIST(current)->ncaps * sizeof(*cap);

	/* Validate user buffer for this copy size */
	if ((err = check_access((unsigned long)userbuf,
				copy_size,
				MAP_USR_RW_FLAGS, 1)) < 0)
		return err;

	/* Copy capabilities from list to buffer */
	list_foreach_struct(cap, &TASK_CAP_LIST(current)->caps,
			    list) {
		memcpy(userbuf + copy_offset,
		       cap, sizeof(*cap));
		copy_offset += sizeof(*cap);
	}

	return 0;
}

/*
 * Currently shares _all_ capabilities of a task with a
 * collection of threads
 *
 * FIXME: Check ownership for sharing.
 *
 * Currently we don't need to check since we _only_ share
 * the ones from our own private list. If we shared from
 * a collection's list, we would need to check ownership.
 */
int capability_share(unsigned int share_flags)
{
	switch (share_flags) {
	case CAP_SHARE_SPACE:
		cap_list_move(&current->space->cap_list,
			      TASK_CAP_LIST(current));
		break;
	case CAP_SHARE_CONTAINER:
		cap_list_move(&curcont->cap_list,
			      TASK_CAP_LIST(current));
		break;
#if 0
	case CAP_SHARE_CHILD:
		/*
		 * Move own capabilities to paged-children
		 * cap list so that all children can benefit
		 * from our own capabilities.
		 */
		cap_list_move(&current->pager_cap_list,
			      &current->cap_list);
		break;
	case CAP_SHARE_SIBLING: {
		/* Find our pager */
		struct ktcb *pager = tcb_find(current->pagerid);

		/*
		 * Add own capabilities to its
		 * paged-children cap_list
		 */
		cap_list_move(&pager->pager_cap_list,
			      &current->cap_list);
		break;
	}
	case CAP_SHARE_GROUP: {
		struct ktcb *tgroup_leader;

	       	BUG_ON(!(tgroup_leader = tcb_find(current->tgid)));
		cap_list_move(&tgroup_leader->tgroup_cap_list,
			      &current->cap_list);
		break;
	}
#endif
	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * Read, manipulate capabilities. Currently only capability read support.
 */
int sys_capability_control(unsigned int req, unsigned int flags, void *userbuf)
{
	int err;

	/*
	 * Check capability to do a capability operation.
	 * Supported only on current's caps for time being.
	 */
	if ((err = cap_cap_check(current, req, flags)) < 0)
		return err;

	switch(req) {
	/* Return number of capabilities the thread has */
	case CAP_CONTROL_NCAPS:
		if (current != current->pager->tcb)
			return -EPERM;

		if ((err = check_access((unsigned long)userbuf,
					sizeof(int),
					MAP_USR_RW_FLAGS, 1)) < 0)
			return err;

		/* Copy ncaps value. FIXME: This is only a partial list */
		*((int *)userbuf) = TASK_CAP_LIST(current)->ncaps;
		break;

	/* Return all capabilities as an array of capabilities */
	case CAP_CONTROL_READ:
		err = read_task_capabilities(userbuf);
		break;
	case CAP_CONTROL_SHARE:
		err = capability_share(flags);
		break;
	default:
		/* Invalid request id */
		return -EINVAL;
	}
	return 0;
}


#if 0

int capability_grant(struct ktcb *recv, struct ktcb *send, struct capability *cap)
{
	list_del(&cap->list);
	list_add(&cap->list, &recv->cap_list);
}

struct cap_split_desc {
	unsigned int valid;
	unsigned int access;
	unsigned long size;
	unsigned long start;
	unsigned long end;
};

struct capability *capability_diff(struct capability *cap, struct cap_split_desc *split)
{

}

int capability_split(struct ktcb *recv, struct ktcb *send, struct capability *cap, struct cap_split_desc *split)
{
	capability_diff(orig, new, split);
}

int capability_split(struct capability *orig, struct capability *diff, unsigned int valid)
{
	struct capability *new;

	if (!(new = alloc_capability()))
		return -ENOMEM;

	if (valid & FIELD_TO_BIT(struct capability, access)) {
		new->access = orig->access & diff->access;
		orig->access &= ~diff->access;
	}
	if (valid & FIELD_TO_BIT(struct capability, size)) {
		new->size = orig->size - diff->size;
		orig->size -= diff->size;
	}
	if (valid & FIELD_TO_BIT(struct capability, start)) {
		/* This gets complicated, may return -ENOSYS for now */
		memcap_unmap(cap, diff->start, diff->end);
		new->size = orig->size - split->size;
		orig->size -= split->size;
	}
}

#endif
