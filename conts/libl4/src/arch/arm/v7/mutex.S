/*
 * Copyright (C) 2009 Bahadir Balban
 */

#include <l4lib/arch/arm/asm.h>
#include <l4lib/mutex.h>

/*
 * @r0 = address of mutex word
 * @r1 = unique tid of current thread
 */
BEGIN_PROC(__l4_mutex_lock)
1:
	ldrex	r2, [r0]			@ Load value
	cmp	r2, #L4_MUTEX_UNLOCKED		@ Decide what state lock will be if we succeed in a store
	movne	r2, #L4_MUTEX_CONTENDED
	moveq	r2, #L4_MUTEX_SUCCESS
	strex	r3, r1, [r0]			@ Store prospective lock state
	cmp	r3, #0				@ If not successful
						@ No WFE. Whatif this were between 2 threads running on the same cpu
	bne	1b				@ Retry and decide again on the prospective lock state.
	dsb
	mov	r0, r2
	mov	pc, lr
END_PROC(__l4_mutex_lock)

/*
 * @r0 = address of mutex word
 * @r1 = unique tid of current thread
 */
BEGIN_PROC(__l4_mutex_unlock)
	dsb
	push	{r4}
	mov	r4, #L4_MUTEX_UNLOCKED
1:
	ldrex	r2, [r0]
	cmp	r2, r1
	moveq	r3, #L4_MUTEX_SUCCESS
	movne	r3, #L4_MUTEX_CONTENDED
	strex	r2, r4, [r0]
	cmp	r2, #0
	bne	1b
	mov	r0, r3
	pop	{r4}
	mov	pc, lr
END_PROC(__l4_mutex_unlock)


