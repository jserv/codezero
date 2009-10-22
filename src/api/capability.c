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

	/* Determine size of pager capabilities */
	copy_size = current->cap_list.ncaps * sizeof(*cap);

	/* Validate user buffer for this copy size */
	if ((err = check_access((unsigned long)userbuf,
				copy_size,
				MAP_USR_RW_FLAGS, 1)) < 0)
		return err;

	/* Copy capabilities from list to buffer */
	list_foreach_struct(cap, &current->cap_list.caps,
			    list) {
		memcpy(userbuf + copy_offset,
		       cap, sizeof(*cap));
		copy_offset += sizeof(*cap);
	}

	return 0;
}

/*
 * Currently shares _all_ capabilities of a task
 * with a collection of threads
 *
 * FIXME: Check ownership for sharing.
 */
int capability_share(unsigned int share_flags)
{
	switch (share_flags) {
	case CAP_SHARE_SPACE:
		cap_list_move(&current->space->cap_list,
			      &current->cap_list);
		break;
	case CAP_SHARE_CONTAINER:
		cap_list_move(&curcont->cap_list,
			      &current->cap_list);
		break;
	case CAP_SHARE_GROUP: {
		struct ktcb *tgr_leader;

	       	BUG_ON(!(tgr_leader = tcb_find(current->tgid)));
		cap_list_move(&tgr_leader->cap_list,
			      &current->cap_list);
		break;
	}
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

	switch(req) {
	/* Return number of capabilities the thread has */
	case CAP_CONTROL_NCAPS:
		if (current != current->pager->tcb)
			return -EPERM;

		if ((err = check_access((unsigned long)userbuf,
					sizeof(int),
					MAP_USR_RW_FLAGS, 1)) < 0)
			return err;

		/* Copy ncaps value */
		*((int *)userbuf) = current->cap_list.ncaps;
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
	if (split->valid & FIELD_TO_BIT(struct cap_split_desc, access)) {
		new->access = orig->access & split->access;
		orig->access &= ~split->access;
	}
	if (split->valid & FIELD_TO_BIT(struct cap_split_desc, size)) {
		new->size = orig->size - split->size;
		orig->size -= split->size;
	}
	if (split->valid & FIELD_TO_BIT(struct cap_split_desc, start)) {
		memcap_unmap(cap, split->start, split->end);
		new->size = orig->size - split->size;
		orig->size -= split->size;
	}

}

int capability_split(struct ktcb *recv, struct ktcb *send, struct capability *cap, struct cap_split_desc *split)
{
	capability_diff(orig, new, split);
}
#endif
