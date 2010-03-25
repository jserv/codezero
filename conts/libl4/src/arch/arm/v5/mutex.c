/*
 * Copyright (C) 2009-2010 B Labs Ltd.
 * Author: Bahadir Balban
 */

#include L4LIB_INC_ARCH(asm.h)
#include <l4lib/mutex.h>
#include <l4lib/types.h>
#include INC_SUBARCH(irq.h)
#include L4LIB_INC_ARCH(syslib.h)	/* for BUG/BUG_ON,  */

/*
 * NOTES:
 *
 * Recap on swp:
 *
 * swp rx, ry, [rz]
 *
 * In one instruction:
 *
 * 1) Stores the value in ry into location pointed by rz.
 * 2) Loads the value in the location of rz into rx.
 * By doing so, in one instruction one can attempt to lock
 * a word, and discover whether it was already locked.
 *
 * Why use tid of thread to lock mutex instead of
 * a single lock value?
 *
 * Because in one atomic instruction, not only the locking attempt
 * should be able to indicate whether it is locked, but also
 * the contentions. A unified lock value would not be sufficient.
 * The only way to indicate a contended lock is to store the
 * unique TID of the locker.
 */

/*
 * Any non-negative value that is a potential TID
 * (including 0) means mutex is locked.
 */


int __l4_mutex_lock(void *m, l4id_t tid)
{
	unsigned int tmp;

	__asm__ __volatile__(
		"swp %0, %1, [%2]"
		: "=r" (tmp)
		: "r"(tid), "r" (m)
		: "memory"
	);

	if (tmp == L4_MUTEX_UNLOCKED)
		return L4_MUTEX_SUCCESS;

	return L4_MUTEX_CONTENDED;
}


int __l4_mutex_unlock(void *m, l4id_t tid)
{
	unsigned int tmp, tmp2 = L4_MUTEX_UNLOCKED;

	__asm__ __volatile__(
		"swp %0, %1, [%2]"
		: "=r" (tmp)
		: "r" (tmp2), "r"(m)
		: "memory"
	);

	BUG_ON(tmp == L4_MUTEX_UNLOCKED);

	if (tmp == tid)
		return L4_MUTEX_SUCCESS;

	return L4_MUTEX_CONTENDED;
}

u8 l4_atomic_dest_readb(unsigned long *location)
{
#if 0
	unsigned int tmp;
	__asm__ __volatile__ (
		"swpb	r0, r2, [r1] \n"
		: "=r"(tmp)
		: "r"(location), "r"(0)
		: "memory"
	);
	return (u8)tmp;
#endif

	unsigned int tmp;
       // unsigned long state;
       // irq_local_disable_save(&state);

        tmp = *location;
        *location = 0;
        
        //irq_local_restore(state);

        return (u8)tmp;

}
