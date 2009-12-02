/*
 * UTCB management in libl4thread.
 *
 * Copyright © 2009 B Labs Ltd.
 */
#include <stdio.h>
#include <l4lib/addr.h>
#include <l4lib/thread/utcb-common.h>
#include <malloc/malloc.h>

/* Globally disjoint utcb virtual region pool */
static struct address_pool utcb_region_pool;

int utcb_pool_init(unsigned long utcb_start, unsigned long utcb_end)
{
	int err;

	/* Initialise the global utcb virtual address pool */
	if ((err = address_pool_init(&utcb_region_pool,
					utcb_start, utcb_end,
					PAGE_SIZE) < 0)) {
		printf("UTCB address pool initialisation failed.\n");
		return err;
	}

	return 0;
}

static inline void *utcb_new_address(int nitems)
{
	return address_new(&utcb_region_pool, nitems, PAGE_SIZE);
}

static inline int utcb_delete_address(void *utcb_address, int nitems)
{
	return address_del(&utcb_region_pool, utcb_address, nitems, PAGE_SIZE);
}

/* Return an empty utcb slot in this descriptor */
unsigned long utcb_new_slot(struct l4lib_utcb_desc *desc)
{
	int slot;

	if ((slot = id_new(desc->slots)) < 0)
		return 0;
	else
		return desc->utcb_base + (unsigned long)slot * UTCB_SIZE;
}

int utcb_delete_slot(struct l4lib_utcb_desc *desc, unsigned long address)
{
	BUG_ON(id_del(desc->slots, (address - desc->utcb_base)
		      / UTCB_SIZE) < 0);
	return 0;
}

struct l4lib_utcb_desc *utcb_new_desc(void)
{
	struct l4lib_utcb_desc *d;

	/* Allocate a new descriptor */
	if (!(d	= kzalloc(sizeof(*d))))
		return 0;

	link_init(&d->list);

	/* We currently assume UTCB is smaller than PAGE_SIZE */
	BUG_ON(UTCB_SIZE > PAGE_SIZE);

	/* Initialise utcb slots */
	d->slots = id_pool_new_init(PAGE_SIZE / UTCB_SIZE);

	/* Obtain a new and unique utcb base */
	/* FIXME: Use variable size than a page */
	if (!(d->utcb_base = (unsigned long)utcb_new_address(1))) {
		kfree(d->slots);
		kfree(d);
		return 0;
	}

	return d;
}

int utcb_delete_desc(struct l4lib_utcb_desc *desc)
{
	/* Return descriptor address */
	utcb_delete_address((void *)desc->utcb_base, 1);

	/* Free the descriptor */
	kfree(desc);

	return 0;
}
