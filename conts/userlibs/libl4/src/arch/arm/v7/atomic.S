/*
 * Copyright (C) 2010 B Labs
 *
 * Author: Bahadir Balban
 */

#include <l4lib/arch/arm/asm.h>

/*
 * Atomically and destructively reads a byte. E.g.
 * byte is read and zero is written back. This is
 * useful on reading irq counts
 *
 * @r0 = byte address
 */
BEGIN_PROC(l4_atomic_dest_readb)
	mov	r2, #0
1:
	ldrexb	r1, [r0]
	strexb	r3, r2, [r0]
	cmp	r3, #0
	bne 	1b
	mov	r0, r1
	mov	pc, lr
END_PROC(l4_atomic_dest_readb)




