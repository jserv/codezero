


#include INC_ARCH(asm.h)

/*
 * r0 = unsigned long *state, stores current irq state.
 */
BEGIN_PROC(irq_local_disable_save)
	mov	r1, r0		@ Move address of state to r1
	mrs	r2, cpsr_fc	@ Read cpsr
	orr	r3, r2, #0x80	@ Disable irq bit in cpsr
	msr	cpsr_fc, r3	@ Write to cpsr
	str	r2, [r1]	@ Store original cpsr in r2 to state
	mov	pc, lr
END_PROC(irq_local_disable_save)

/*
 * r0 = byte address to read from.
 */
BEGIN_PROC(l4_atomic_dest_readb)
	mov	r1, r0		@ Move byte address to r1
	mov	r2, #0		@ Move 0 to r2
	swpb	r0, r2, [r1]	@ Write 0 to byte location, while reading its value to r0
	mov	pc, lr		@ Return byte location value
END_PROC(l4_atomic_dest_readb)

