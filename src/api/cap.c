/*
 * Capability manipulation syscall.
 *
 * The entry to Codezero security
 * mechanisms.
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/api/capability.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/generic/container.h>
#include <l4/generic/tcb.h>
#include <l4/api/errno.h>
#include INC_API(syscall.h)

/*
 * FIXME: This is reading only a single list
 * there may be more than one.
 */
int cap_read_all(struct capability *caparray)
{
	struct capability *cap;
	int total_size, capidx = 0;
	int err;

	/*
	 * Determine size of pager capabilities
	 * (FIXME: partial!)
	 */
	total_size = TASK_CAP_LIST(current)->ncaps
		   * sizeof(*cap);
	if ((err = check_access((unsigned long)caparray,
				total_size,
				MAP_USR_RW_FLAGS, 1)) < 0)
		return err;

	/*
	 * Currently only pagers can
	 * read their own capabilities
	 */
	if (current->tid != current->pagerid)
		return -EPERM;

	/* Copy capabilities from list to buffer */
	list_foreach_struct(cap,
			    &TASK_CAP_LIST(current)->caps,
			    list) {
		memcpy(&caparray[capidx], cap, sizeof(*cap));
		capidx++;
	}

	return 0;
}

/*
 * Shares single cap. If you are sharing, there is
 * only one target that makes sense, that is your
 * own container.
 */
int cap_share_single(l4id_t capid)
{
	struct capability *cap;

	if (!(cap = cap_find_byid(capid)))
		return -EEXIST;

	if (cap->owner != current->tid)
		return -EPERM;

	/* First remove it from its list */
	list_remove(&cap->list);

	/* Place it where it is shared */
	cap_list_insert(cap, &curcont->cap_list);

	return 0;
}

/* Shares the whole list */
int cap_share_all(void)
{
	cap_list_move(&curcont->cap_list,
		      TASK_CAP_LIST(current));
	return 0;
}

int cap_share(l4id_t capid, unsigned int flags)
{
	if (flags & CAP_SHARE_SINGLE)
		cap_share_single(capid);
	else if (flags & CAP_SHARE_ALL)
		cap_share_all();
	else
		return -EINVAL;
	return 0;
}

/* Grants all caps */
int cap_grant_all(l4id_t tid, unsigned int flags)
{
	struct ktcb *target;
	struct capability *cap_head, *cap;
	int err;

	if (!(target = tcb_find(tid)))
		return -ESRCH;

	/* Detach all caps */
	cap_head = cap_list_detach(TASK_CAP_LIST(current));

	list_foreach_struct(cap, &cap_head->list, list) {
		/* Change ownership */
		cap->owner = target->tid;

		/* Make immutable if GRANT_IMMUTABLE given */
		if (flags & CAP_GRANT_IMMUTABLE) {
			cap->access &= ~CAP_GENERIC_MASK;
			cap->access |= CAP_IMMUTABLE;
		}

		/*
		 * Sanity check: granted cap cannot have used
		 * quantity. Otherwise how else the original
		 * users of the cap free them?
		 */
		if (cap->used) {
			err = -EPERM;
			goto out_err;
		}
	}

	/* Attach all to target */
	cap_list_attach(cap_head, TASK_CAP_LIST(target));
	return 0;

out_err:
	/* Attach it back to original */
	cap_list_attach(cap_head, TASK_CAP_LIST(current));
	return err;
}

int cap_grant_single(l4id_t capid, l4id_t tid, unsigned int flags)
{
	struct capability *cap;
	struct ktcb *target;

	if (!(cap = cap_find_byid(capid)))
		return -EEXIST;

	if (!(target = tcb_find(tid)))
		return -ESRCH;

	if (cap->owner != current->tid)
		return -EPERM;

	/* Granted cap cannot have used quantity */
	if (cap->used)
		return -EPERM;

	/* First remove it from its list */
	list_remove(&cap->list);

	/* Change ownership */
	cap->owner = target->tid;

	/* Make immutable if GRANT_IMMUTABLE given */
	if (flags & CAP_GRANT_IMMUTABLE) {
		cap->access &= ~CAP_GENERIC_MASK;
		cap->access |= CAP_IMMUTABLE;
	}

	/* Place it where it is granted */
	cap_list_insert(cap, TASK_CAP_LIST(target));

	return 0;
}

int cap_grant(l4id_t capid, l4id_t tid, unsigned int flags)
{
	if (flags & CAP_GRANT_SINGLE)
		cap_grant_single(capid, tid, flags);
	else if (flags & CAP_GRANT_ALL)
		cap_grant_all(tid, flags);
	else
		return -EINVAL;
	return 0;
}

int cap_deduce_rtype(struct capability *orig, struct capability *new)
{
	struct ktcb *target;
	struct address_space *sp;

	/* An rtype deduction can only be to a space or thread */
	switch (cap_rtype(new)) {
	case CAP_RTYPE_SPACE:
		/* Check containment right */
		if (cap_rtype(orig) != CAP_RTYPE_CONTAINER)
			return -ENOCAP;

		/*
		 * Find out if this space exists in this
		 * container.
		 *
		 * Note address space search is local only.
		 * Only thread searches are global.
		 */
		if (!(sp = address_space_find(new->resid)))
			return -ENOCAP;

		/* Success. Assign new type to original cap */
		cap_set_rtype(orig, cap_rtype(new));

		/* Assign the space id to orig cap */
		orig->resid = sp->spid;
		break;
	case CAP_RTYPE_THREAD:
		/* Find the thread */
		if (!(target = tcb_find(new->resid)))
			return -ENOCAP;

		/* Check containment */
		if (cap_rtype(orig) == CAP_RTYPE_SPACE) {
			if (orig->resid != target->space->spid)
				return -ENOCAP;
		} else if (cap_rtype(orig) == CAP_RTYPE_CONTAINER) {
			if(orig->resid != target->container->cid)
				return -ENOCAP;
		} else
			return -ENOCAP;

		/* Success. Assign new type to original cap */
		cap_set_rtype(orig, cap_rtype(new));

		/* Assign the space id to orig cap */
		orig->resid = target->tid;
		break;
	default:
		return -ENOCAP;
	}
	return 0;
}

/*
 * Deduction can be by access permissions, start, end, size
 * fields, or the target resource type. Inter-container
 * deduction is not allowed.
 *
 * Target resource deduction denotes reducing the applicable
 * space of the target, e.g. from a container to a space in
 * that container.
 *
 * NOTE: If there is no target deduction, you cannot change
 * resid, as this is forbidden.
 *
 * Imagine a space cap, it cannot be deduced to become applicable
 * to another space, i.e a space is in same privilege level.
 * But a container-wide cap can be reduced to be applied on
 * a space in that container (thus changing the resid to that
 * space's id)
 *
 * capid: Id of original capability
 * new: Userspace pointer to new state of capability
 * that is desired.
 *
 * orig = deduced;
 */
int cap_deduce(struct capability *new)
{
	struct capability *orig;
	int ret;

	/* Find original capability */
	if (!(orig = cap_find_byid(new->capid)))
		return -EEXIST;

	/* Check that caller is owner */
	if (orig->owner != current->tid)
		return -ENOCAP;

	/* Check that it is deducable */
	if (!(orig->access & CAP_CHANGEABLE))
		return -ENOCAP;

	/* Check target resource deduction */
	if (cap_rtype(new) != cap_rtype(orig))
		if ((ret = cap_deduce_rtype(orig, new)) < 0)
			return ret;

	/* Check permissions for deduction */
	if (orig->access) {
		/* New cannot have more bits than original */
		if ((orig->access & new->access) != new->access)
			return -EINVAL;
		/* New cannot make original redundant */
		if (new->access == 0)
			return -EINVAL;

		/* Deduce bits of orig */
		orig->access &= new->access;
	} else if (new->access)
		return -EINVAL;

	/* Check size for deduction */
	if (orig->size) {
		/* New can't have more, or make original redundant */
		if (new->size >= orig->size)
			return -EINVAL;

		/*
		 * Can't make reduction on used ones, so there
		 * must be enough available ones
		 */
		if (new->size < orig->used)
			return -EPERM;
		orig->size = new->size;
	} else if (new->size)
		return -EINVAL;

	/* Range-like permissions can't be deduced */
	if (orig->start || orig->end) {
		if (orig->start != new->start ||
		    orig->end != new->end)
			return -EPERM;
	} else if (new->start || new->end)
		return -EINVAL;

	/* Ensure orig and new are the same */
	BUG_ON(orig->capid != new->capid);
	BUG_ON(orig->resid != new->resid);
	BUG_ON(orig->owner != new->owner);
	BUG_ON(orig->type != new->type);
	BUG_ON(orig->access != new->access);
	BUG_ON(orig->start != new->start);
	BUG_ON(orig->end != new->end);
	BUG_ON(orig->size != new->size);
	BUG_ON(orig->used != new->used);

	return 0;
}

/*
 * Splits a capability
 *
 * Pools of typed memory objects can't be replicated, and
 * deduced that way, as replication would temporarily double
 * their size. So they are split in place.
 *
 * Splitting occurs by diff'ing resources possessed between
 * capabilities.
 *
 * capid: Original capability that is valid.
 * diff: New capability that we want to split out.
 *
 * orig = orig - diff;
 * new = diff;
 */
int cap_split(struct capability *diff)
{
	struct capability *orig, *new;

	/* Find original capability */
	if (!(orig = cap_find_byid(diff->capid)))
		return -EEXIST;

	/* Check target type/resid/owner is the same */
	if (orig->type != diff->type ||
	    orig->resid != diff->resid ||
	    orig->owner != diff->owner)
		return -EINVAL;

	/* Check that caller is owner */
	if (orig->owner != current->tid)
		return -ENOCAP;

	/* Check that it is splitable */
	if (!(orig->access & CAP_CHANGEABLE))
		return -ENOCAP;

	/* Create new */
	if (!(new = capability_create()))
		return -ENOCAP;

	/* Check access bits usage and split */
	if (orig->access) {
		/* Split one can't have more bits than original */
		if ((orig->access & diff->access) != diff->access) {
			free_capability(new);
			return -EINVAL;
		}

		/* Split one cannot make original redundant */
		if ((orig->access & ~diff->access) == 0) {
			free_capability(new);
			return -EINVAL;
		}

		/* Subtract given access permissions */
		orig->access &= ~diff->access;

		/* Assign given perms to new capability */
		new->access = diff->access;
	} else if (new->access)
		return -EINVAL;

	/* Check size usage and split */
	if (orig->size) {
		/*
		 * Split one can't have more,
		 * or make original redundant
		 */
		if (diff->size >= orig->size) {
			free_capability(new);
			return -EINVAL;
		}

		/* Split one must be clean i.e. all unused */
		if (orig->size - orig->used < diff->size) {
			free_capability(new);
			return -EPERM;
		}

		orig->size -= diff->size;
		new->size = diff->size;
		new->used = 0;
	} else if (new->size)
		return -EINVAL;


	/* Check range usage but don't split if requested */
	if (orig->start || orig->end) {
		/* Range-like permissions can't be deduced */
		if (orig->start != new->start ||
		    orig->end != new->end) {
			free_capability(new);
			return -EPERM;
		}
	} else if (new->start || new->end)
		return -EINVAL;



	/* Copy other fields */
	new->type = orig->type;
	new->resid = orig->resid;
	new->owner = orig->owner;

	/* Add the new capability to the most private list */
	cap_list_insert(new, TASK_CAP_LIST(current));

	/* Copy the new one to diff for userspace */
	BUG_ON(new->resid != diff->resid);
	BUG_ON(new->owner != diff->owner);
	BUG_ON(new->type != diff->type);
	BUG_ON(new->access != diff->access);
	BUG_ON(new->start != diff->start);
	BUG_ON(new->end != diff->end);
	BUG_ON(new->size != diff->size);
	BUG_ON(new->used != diff->used);
	diff->capid = new->capid;

	return 0;
}

/*
 * Replicates an existing capability. This is for expanding
 * capabilities to managed children.
 *
 * After replication, a duplicate capability exists in the
 * system, but as it is not a quantity, this does not increase
 * the capabilities of the caller in any way.
 */
int cap_replicate(struct capability *dupl)
{
	struct capability *new, *orig;

	/* Find original capability */
	if (!(orig = cap_find_byid(dupl->capid)))
		return -EEXIST;

	/* Check that caller is owner */
	if (orig->owner != current->tid)
		return -ENOCAP;

	/* Check that it is replicable */
	if (!(orig->access & CAP_REPLICABLE))
		return -ENOCAP;

	 /* Quantitative types must not be replicable */
	if (cap_type(orig) == CAP_TYPE_QUANTITY) {
		printk("Cont %d: FATAL: Capability (%d) "
		       "is quantitative but also replicable\n",
		       curcont->cid, orig->capid);
		/* FIXME: Should rule this out as a CML2 requirement */
		BUG();
	}

	/* Replicate it */
	if (!(new = capability_create()))
		return -ENOCAP;

	/* Copy all except capid & listptrs */
	dupl->resid = new->resid = orig->resid;
	dupl->owner = new->owner = orig->owner;
	dupl->type = new->type = orig->type;
	dupl->access = new->access = orig->access;
	dupl->start = new->start = orig->start;
	dupl->end = new->end = orig->end;
	dupl->size = new->size = orig->size;
	dupl->used = new->used = orig->used;

	/* Copy new fields */
	dupl->capid = new->capid;

	/* Add it to most private list */
	cap_list_insert(new, TASK_CAP_LIST(current));

	return 0;
}

/*
 * Read, manipulate capabilities. Currently only capability read support.
 */
int sys_capability_control(unsigned int req, unsigned int flags,
			   l4id_t capid, l4id_t target, void *userbuf)
{
	int err = 0;

	/*
	 * Check capability to do a capability operation.
	 * Supported only on current's caps for time being.
	 */
	if ((err = cap_cap_check(current, req, flags)) < 0)
		return err;

	switch(req) {
	case CAP_CONTROL_NCAPS:
		/* Return number of caps */
		if (current->tid != current->pagerid)
			return -EPERM;

		if ((err = check_access((unsigned long)userbuf,
					sizeof(int),
					MAP_USR_RW_FLAGS, 1)) < 0)
			return err;

		/* Copy ncaps value. FIXME: This is only a partial list */
		*((int *)userbuf) = TASK_CAP_LIST(current)->ncaps;
		break;
	case CAP_CONTROL_READ: {
		/* Return all capabilities as an array of capabilities */
		err = cap_read_all((struct capability *)userbuf);
		break;
	}
	case CAP_CONTROL_SHARE:
		err = cap_share(capid, flags);
		break;
	case CAP_CONTROL_GRANT:
		err = cap_grant(flags, capid, target);
		break;
	case CAP_CONTROL_SPLIT:
		err = cap_split((struct capability *)userbuf);
		break;
	case CAP_CONTROL_REPLICATE:
		if ((err = check_access((unsigned long)userbuf,
					sizeof(int),
					MAP_USR_RW_FLAGS, 1)) < 0)
			return err;

		err = cap_replicate((struct capability *)userbuf);
		break;
	case CAP_CONTROL_DEDUCE:
		if ((err = check_access((unsigned long)userbuf,
					sizeof(int),
					MAP_USR_RW_FLAGS, 1)) < 0)
			return err;

		err = cap_deduce((struct capability *)userbuf);
		break;
	default:
		/* Invalid request id */
		return -EINVAL;
	}

	return err;
}

