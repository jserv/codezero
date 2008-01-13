/*
 * Used for thread and space ids.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/printk.h>
#include <l4/lib/idpool.h>
#include <l4/generic/kmalloc.h>
#include INC_GLUE(memory.h)

struct id_pool *id_pool_new_init(int totalbits)
{
	int nwords = BITWISE_GETWORD(totalbits);
	struct id_pool *new = kzalloc((nwords * SZ_WORD)
				      + sizeof(struct id_pool));
	new->nwords = nwords;
	return new;
}

int id_new(struct id_pool *pool)
{
	int id = find_and_set_first_free_bit(pool->bitmap,
					     pool->nwords * WORD_BITS);
	BUG_ON(id < 0);
	return id;
}

int id_del(struct id_pool *pool, int id)
{
	int ret = check_and_clear_bit(pool->bitmap, id);

	BUG_ON(ret < 0);
	return ret;
}

