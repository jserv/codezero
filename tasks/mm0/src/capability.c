/*
 * Pager's capabilities for kernel resources
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <bootm.h>
#include <init.h>
#include <l4/lib/list.h>
#include <capability.h>
#include <l4/api/capability.h>
#include <l4lib/arch/syscalls.h>
#include <l4/generic/cap-types.h>	/* TODO: Move this to API */
#include <lib/malloc.h>

struct cap_list capability_list;

__initdata static struct capability *caparray;
__initdata static int total_caps = 0;

/* Copy all init-memory allocated capabilities */
void copy_boot_capabilities(struct initdata *initdata)
{
	struct capability *cap;

	for (int i = 0; i < total_caps; i++) {
		cap = kzalloc(sizeof(struct capability));

		/* This copies kernel-allocated unique cap id as well */
		memcpy(cap, &caparray[i], sizeof(struct capability));

		/* Initialize capability list */
		link_init(&cap->list);

		/* Add capability to global cap list */
		list_insert(&capability_list.caps, &cap->list);
	}
}

int read_kernel_capabilities(struct initdata *initdata)
{
	int ncaps;
	int err;

	/* Read number of capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_NCAPS, 0, &ncaps)) < 0) {
		printf("l4_capability_control() reading # of capabilities failed.\n"
		       "Could not complete CAP_CONTROL_NCAPS request.\n");
		goto error;
	}
	total_caps = ncaps;

	/* Allocate array of caps from boot memory */
	caparray = alloc_bootmem(sizeof(struct capability) * ncaps, 0);

	/* Read all capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_READ_CAPS, 0, caparray)) < 0) {
		printf("l4_capability_control() reading of capabilities failed.\n"
		       "Could not complete CAP_CONTROL_READ_CAPS request.\n");
		goto error;
	}

	/* Set up initdata pointer to important capabilities */
	initdata->bootcaps = caparray;
	for (int i = 0; i < ncaps; i++) {
		/*
		 * TODO: There may be multiple physmem caps!
		 * This really needs to be considered as multiple
		 * membanks!!!
		 */
		if ((caparray[i].type & CAP_RTYPE_MASK)
		    == CAP_RTYPE_PHYSMEM) {
			initdata->physmem = &caparray[i];
			return 0;
		}
	}
	printf("%s: Error, pager has no physmem capability defined.\n",
		__TASKNAME__);
	goto error;

	return 0;

error:
	BUG();
}

