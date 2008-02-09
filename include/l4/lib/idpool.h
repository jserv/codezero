#ifndef __IDPOOL_H__
#define __IDPOOL_H__

#include <l4/lib/bit.h>
#include <l4/lib/spinlock.h>

struct id_pool {
	struct spinlock lock;
	int nwords;
	u32 bitmap[];
};

struct id_pool *id_pool_new_init(int mapsize);
int id_new(struct id_pool *pool);
int id_del(struct id_pool *pool, int id);
int id_get(struct id_pool *pool, int id);

#endif /* __IDPOOL_H__ */
