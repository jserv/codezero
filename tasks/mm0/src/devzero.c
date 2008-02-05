/*
 * Handling of the special zero page.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <memory.h>
#include <mm/alloc_page.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <string.h>
#include INC_GLUE(memory.h)
#include INC_SUBARCH(mm.h)
#include <l4/generic/space.h>
#include <arch/mm.h>

static void *zpage_p;
static struct page *zpage;
static struct vm_file devzero;

void init_zero_page(void)
{
	void *zpage_v;
	zpage_p = alloc_page(1);
	zpage = phys_to_page(zpage_p);

	/* Map it to self */
	zpage_v = l4_map_helper(zpage_p, 1);

	/* Zero it */
	memset(zpage_v, 0, PAGE_SIZE);

	/* Unmap it */
	l4_unmap_helper(zpage_v, 1);

	/* Update page struct. All other fields are zero */
	zpage->count++;
}

void init_devzero(void)
{
	init_zero_page();

	INIT_LIST_HEAD(&devzero.list);
	INIT_LIST_HEAD(&devzero.page_cache_list);
	devzero.length = (unsigned int)-1;
	devzero.inode.i_addr = -1;
}

struct vm_file *get_devzero(void)
{
	return &devzero;
}

void *get_zero_page(void)
{
	zpage->count++;
	return zpage_p;
}

void put_zero_page(void)
{
	zpage->count--;
	BUG_ON(zpage->count < 0);
}

