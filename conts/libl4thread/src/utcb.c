#include <stdio.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/exregs.h>
#include <errno.h>
#include <malloc/malloc.h>
#include <utcb.h>

static unsigned long utcb_end_max_addr;

#if defined(MAPPING_ENABLE)
static int setup_utcb_table(unsigned long utcb_start_addr,
				unsigned long utcb_end_addr)
{
	unsigned long utcb_range_size;
	unsigned long utcb_table_size;
	int total_entry;

	/*
	 * Establish the utcb_table.
	 *	1. Calculate the size of the table.
	 *	2. Calculate the number of entries.
	 *	3. Allocate it.
	 */
	utcb_range_size = utcb_end_addr - utcb_start_addr;
	total_entry = utcb_range_size / UTCB_SIZE;
	utcb_table_size = sizeof(struct utcb_entry) * total_entry;
	if (!(utcb_table_ptr = kzalloc(utcb_table_size))) {
		printf("libl4thread: There is not enough memory for the utcb "
			"mapping table of size(%d)!\n", utcb_table_size);
		return -ENOMEM;
	}
	return 0;
}
#endif

static int set_utcb_addr(void)
{
	struct exregs_data exregs;
	unsigned long utcb_addr;
	struct task_ids ids;
	int err;

	l4_getid(&ids);
	// FIXME: its tid must be 0.
#if !defined(MAPPING_ENABLE)
	udesc_ptr = utcb_new_desc();
	utcb_addr = get_utcb_addr();
#else
	{
		struct utcb_desc *udesc;
		udesc = utcb_new_desc();
		utcb_table_ptr[ids.tid].udesc = udesc;
		utcb_addr = get_utcb_addr(&ids, &ids);
	}
#endif
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

#if defined(MAPPING_ENABLE)
	setup_utcb_table(utcb_start_addr, utcb_end_addr);
#endif
	utcb_end_max_addr = utcb_end_addr;

	/* The very first thread's utcb address is assigned. */
	if ((err = set_utcb_addr()) < 0)
		return err;

	return 0;
}

#if !defined(MAPPING_ENABLE)
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
#else
unsigned long get_utcb_addr(struct task_ids *parent_id,
				struct task_ids *child_id)
{
	struct utcb_entry uentry;
	unsigned long utcb_addr;

	/* Get the entry for this thread. */
	uentry = utcb_table_ptr[parent_id->tid];

	/* Get a new utcb virtual address. */
	if (!(utcb_addr = utcb_new_slot(uentry.udesc))) {
		/* No slot for this desc, get a new desc. */
		struct utcb_desc *udesc;
		udesc = utcb_new_desc();
		utcb_addr = utcb_new_slot(udesc);
		uentry.udesc = udesc;
	}
	/* Place the entry into the utcb_table. */
	utcb_table_ptr[child_id->tid].udesc = uentry.udesc;
	utcb_table_ptr[parent_id->tid].udesc = uentry.udesc;

	if (utcb_addr >= utcb_end_max_addr)
		return 0;

	return utcb_addr;
}
#endif

/*void destroy_utcb(void) {}*/

