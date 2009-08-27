/*
 * Used for thread and space ids.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <lib/idpool.h>
// #include <kmalloc/kmalloc.h> --> This requires page allocation to grow/shrink.
#include <lib/malloc.h> // --> This is a local library that statically allocates its heap.
#include <l4/macros.h>
#include INC_GLUE(memory.h)
#include <printf.h>
#include <l4/api/errno.h>

struct id_pool *id_pool_new_init(int totalbits)
{
	int nwords = BITWISE_GETWORD(totalbits);
	struct id_pool *new = kzalloc((nwords * SZ_WORD)
				      + sizeof(struct id_pool));
	if (!new)
		return PTR_ERR(-ENOMEM);
	new->nwords = nwords;
	return new;
}

int id_new(struct id_pool *pool)
{
	int id = find_and_set_first_free_bit(pool->bitmap,
					     pool->nwords * WORD_BITS);
	if (id < 0)
		printf("%s: Warning! New id alloc failed\n", __FUNCTION__);
	return id;
}

/* This finds n contiguous free ids, allocates and returns the first one */
int ids_new_contiguous(struct id_pool *pool, int numids)
{
	int id = find_and_set_first_free_contig_bits(pool->bitmap,
						     pool->nwords *WORD_BITS,
						     numids);
	if (id < 0)
		printf("%s: Warning! New id alloc failed\n", __FUNCTION__);
	return id;
}

/* This deletes a list of contiguous ids given the first one and number of ids */
int ids_del_contiguous(struct id_pool *pool, int first, int numids)
{
	int ret;

	if (pool->nwords * WORD_BITS < first + numids)
		return -1;
	if ((ret = check_and_clear_contig_bits(pool->bitmap, first, numids)))
		printf("%s: Error: Invalid argument range.\n", __FUNCTION__);
	return ret;
}

int id_del(struct id_pool *pool, int id)
{
	int ret;

	if (pool->nwords * WORD_BITS < id)
		return -1;

	if ((ret = check_and_clear_bit(pool->bitmap, id) < 0))
		printf("%s: Error: Could not delete id.\n", __FUNCTION__);
	return ret;
}

/* Return a specific id, if available */
int id_get(struct id_pool *pool, int id)
{
	int ret;

	ret = check_and_set_bit(pool->bitmap, id);

	if (ret < 0)
		return ret;
	else
		return id;
}

