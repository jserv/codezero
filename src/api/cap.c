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
 * Read all capabilitites of the current process.
 * This includes the private ones as well as
 * ones shared by other tasks that the task has
 * rights to but doesn't own.
 */
int cap_read_all(struct capability *caparray)
{
	struct capability *cap;
	int capidx = 0;

	list_foreach_struct(cap, &current->space->cap_list.caps, list) {
		memcpy(&caparray[capidx], cap, sizeof(*cap));
		capidx++;
	}

	list_foreach_struct(cap, &curcont->cap_list.caps, list) {
		memcpy(&caparray[capidx], cap, sizeof(*cap));
		capidx++;
	}

	return 0;
}

/*
 * Read, manipulate capabilities.
 */
int sys_capability_control(unsigned int req, unsigned int flags, void *userbuf)
{
	int err = 0;

	/* Check access for each request */
	switch(req) {
	case CAP_CONTROL_NCAPS:
		if ((err = check_access((unsigned long)userbuf,
					sizeof(int),
					MAP_USR_RW, 1)) < 0)
			return err;
		break;
	case CAP_CONTROL_READ:
		if ((err = check_access((unsigned long)userbuf,
					cap_count(current) *
					sizeof(struct capability),
					MAP_USR_RW, 1)) < 0)
			return err;
		break;
	default:
		return -EINVAL;
	}

	/* Take action for each request */
	switch(req) {
	case CAP_CONTROL_NCAPS:
		*((int *)userbuf) = cap_count(current);
		break;
	case CAP_CONTROL_READ:
		err = cap_read_all((struct capability *)userbuf);
		break;
	default:
		return -EINVAL;
	}

	return err;
}

