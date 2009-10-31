#ifndef __LIB_SPINLOCK_H__
#define __LIB_SPINLOCK_H__

#include <l4/lib/string.h>
#include <l4/generic/preempt.h>
#include INC_ARCH(exception.h)

struct spinlock {
	unsigned int lock;
};

static inline void spin_lock_init(struct spinlock *s)
{
	memset(s, 0, sizeof(struct spinlock));
}

/*
 * - Guards from deadlock against local processes, but not local irqs.
 * - To be used for synchronising against processes on *other* cpus.
 */
static inline void spin_lock(struct spinlock *s)
{
	preempt_disable();	/* This must disable local preempt */
#if defined(CONFIG_SMP)
	__spin_lock(&s->lock);
#endif
}

static inline void spin_unlock(struct spinlock *s)
{
#if defined(CONFIG_SMP)
	__spin_unlock(&s->lock);
#endif
	preempt_enable();
}

/*
 * - Guards from deadlock against local processes *and* local irqs.
 * - To be used for synchronising against processes and irqs
 *   on other cpus.
 */
static inline void spin_lock_irq(struct spinlock *s,
				 unsigned long state)
{
	irq_local_disable_save(&state);
#if defined(CONFIG_SMP)
	__spin_lock(&s->lock);
#endif
}

static inline void spin_unlock_irq(struct spinlock *s,
				   unsigned long state)
{
#if defined(CONFIG_SMP)
	__spin_unlock(&s->lock);
#endif
	irq_local_restore(state);
}
#endif /* __LIB__SPINLOCK_H__ */
