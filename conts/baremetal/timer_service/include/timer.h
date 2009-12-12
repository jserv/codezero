/*
 * Timer details.
 */
#ifndef __TIMER_H__
#define	__TIMER_H__

#include <l4lib/mutex.h>
#include <l4/lib/list.h>
#include <l4lib/types.h>

/*
 * Timer structure
 */
struct timer {
	int slot;		/* Notify slot on utcb */
	unsigned int base;	/* Virtual base address */
	u64 count;		/* Counter */
	struct link task_list;	/* List of sleepers */
	struct l4_mutex lock;	/* Lock for structure */
	struct capability cap;  /* Capability describing timer */
};

#endif /* __TIMER_H__ */
