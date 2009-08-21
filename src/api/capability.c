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
	copy_size = current->cap_list_ptr->ncaps * sizeof(*cap);

	/* Validate user buffer for this copy size */
	if ((err = check_access((unsigned long)userbuf, copy_size,
				MAP_USR_RW_FLAGS, 1)) < 0)
		return err;

	/* Copy capabilities from list to buffer */
	list_foreach_struct(cap,
			    &current->cap_list_ptr->caps,
			    list) {
		memcpy(userbuf + copy_offset, cap, sizeof(*cap));
		copy_offset += sizeof(*cap);
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

		if ((err = check_access((unsigned long)userbuf, sizeof(int),
					MAP_USR_RW_FLAGS, 1)) < 0)
			return err;

		/* Copy ncaps value */
		*((int *)userbuf) = current->cap_list_ptr->ncaps;
		break;

	/* Return all capabilities as an array of capabilities */
	case CAP_CONTROL_READ_CAPS:
		err = read_task_capabilities(userbuf);
		break;

	default:
		/* Invalid request id */
		return -EINVAL;
	}
	return 0;
}






