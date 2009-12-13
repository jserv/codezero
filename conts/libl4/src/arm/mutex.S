/*
 * Copyright (C) 2009 Bahadir Balban
 */

#include <l4lib/arch/asm.h>
#include <l4lib/mutex.h>

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

/*
 * @r0 = address of mutex word
 * @r1 = unique tid of current thread
 */
BEGIN_PROC(__l4_mutex_lock)
	swp	r2, r1, [r0]
	cmp	r2, #L4_MUTEX_UNLOCKED	@ Was the lock available?
	movne	r0, #L4_MUTEX_CONTENDED	@ Indicate failure
	moveq	r0, #L4_MUTEX_SUCCESS	@ Indicate success
	mov	pc, lr
END_PROC(__l4_mutex_lock)

/*
 * @r0 = address of mutex word
 * @r1 = unique tid of current thread
 */
BEGIN_PROC(__l4_mutex_unlock)
	mov	r3, #L4_MUTEX_UNLOCKED
	swp	r2, r3, [r0]
	cmp	r2, r1			@ Check lock had original tid value
	movne	r0, #L4_MUTEX_CONTENDED	@ Indicate contention
	moveq	r0, #L4_MUTEX_SUCCESS	@ Indicate no contention
	cmp	r2, #L4_MUTEX_UNLOCKED	@ Or - was it already unlocked?
1:
	beq	1b			@ If so busy-spin to indicate bug.
	mov	pc, lr
END_PROC(__l4_mutex_unlock)

/*
 * r0 = byte address to read from.
 */
BEGIN_PROC(l4_atomic_dest_readb)
	mov	r1, r0		@ Move byte address to r1
	mov	r2, #0		@ Move 0 to r2
	swpb	r0, r2, [r1]	@ Write 0 to byte location, while reading its value to r0
	mov	pc, lr		@ Return byte location value
END_PROC(l4_atomic_dest_readb)

