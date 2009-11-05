/*
 * UTCB handling helper routines
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/exregs.h>
#include <errno.h>
#include <malloc/malloc.h>
#include <utcb-common.h>
#include <utcb.h>

/* Static variable definitions */
static unsigned long utcb_end_max_addr;

/* Function definitions */
unsigned long get_utcb_addr(void)
{
	unsigned long utcb_addr;

	if (!(utcb_addr = utcb_new_slot(udesc_ptr))) {
		udesc_ptr = utcb_new_desc();
		utcb_addr = utcb_new_slot(udesc_ptr);
	}

	if (utcb_addr >= utcb_end_max_addr)
		return 0;

	return utcb_addr;
}

static int set_utcb_addr(void)
{
	struct exregs_data exregs;
	unsigned long utcb_addr;
	struct task_ids ids;
	int err;

	l4_getid(&ids);
	// FIXME: its tid must be 0.
	udesc_ptr = utcb_new_desc();
	utcb_addr = get_utcb_addr();

	memset(&exregs, 0, sizeof(exregs));
	exregs_set_utcb(&exregs, (unsigned long)utcb_addr);

	if ((err = l4_exchange_registers(&exregs, ids.tid)) < 0) {
		printf("libl4thread: l4_exchange_registers failed with "
				"(%d)!\n", err);
		return err;
	}

	return 0;
}

int set_utcb_params(unsigned long utcb_start_addr,
			unsigned long utcb_end_addr)
{
	int err;

	/* Ensure that arguments are valid. */
	if (IS_UTCB_SETUP()) {
		printf("libl4thread: You have already called: %s. Simply, "
			"this will have no effect!\n", __FUNCTION__);
		return -EPERM;
	}
	if (!utcb_start_addr || !utcb_end_addr) {
		printf("libl4thread: utcb address range cannot contain "
			"0x00000000 as a start and/or end address(es)!\n");
		return -EINVAL;
	}
	/* Check if the start address is aligned on UTCB_SIZE. */
	if (utcb_start_addr & !UTCB_SIZE) {
		printf("libl4thread: utcb start address must be aligned "
			"on UTCB_SIZE(0x%x)\n", UTCB_SIZE);
		return -EINVAL;
	}
	/* The range must be a valid one. */
	if (utcb_start_addr >= utcb_end_addr) {
		printf("libl4thread: utcb end address must be bigger "
			"than utcb start address!\n");
		return -EINVAL;
	}
	/*
	 * This check guarantees two things:
	 *	1. The range must be multiple of UTCB_SIZE, at least one item.
	 *	2. utcb_end_addr is aligned on UTCB_SIZE
	 */
	if ((utcb_end_addr - utcb_start_addr) % UTCB_SIZE) {
		printf("libl4thread: the given range size must be multiple "
			"of the utcb size(%d)!\n", UTCB_SIZE);
		return -EINVAL;
	}
	/* Arguments passed the validity tests. */

	/* Init utcb virtual address pool */
	utcb_pool_init(utcb_start_addr, utcb_end_addr);

	utcb_end_max_addr = utcb_end_addr;

	/* The very first thread's utcb address is assigned. */
	if ((err = set_utcb_addr()) < 0)
		return err;

	return 0;
}

/*void destroy_utcb(void) {}*/

