/*
 * Basic UART printing.
 */
#include INC_ARCH(asm.h)
#include INC_GLUE(memlayout.h)

#define UART_DATA_OFFSET		0x0

	.macro uart_address rx
	mrc	p15, 0, \rx, c1, c0
	tst	\rx, #1		@ MMU enabled?
	moveq	\rx, #0x10000000
	orreq	\rx, \rx, #0x001F0000
	orreq	\rx, \rx, #0x00001000
	/* FIXME: This offset is incorrect */
	movne	\rx,      #0xf9000000 @#IO_AREA0_VADDR
	addne	\rx, \rx, #PB926_UART0_VOFFSET	@ UART0 page offset from
						@ virtual io area base.
	.endm

	.macro uart_send, ry, rx
	strb	\ry, [\rx, #UART_DATA_OFFSET]
	.endm

	.macro uart_wait, ry, rx
501:
	ldr	\ry, [\rx, #0x18]
	tst	\ry, #1 << 5
	bne	501b
	.endm

	.macro uart_busy, ry, rx
501:
	ldr	\ry, [\rx, #0x18]
	tst	\ry, #1 << 3
	bne	501b
	.endm

		.text
/*
 * Useful debugging routines
 */
BEGIN_PROC(printhex8)
		mov	r1, #8
		b	printhex

BEGIN_PROC(printhex4)
		mov	r1, #4
		b	printhex

BEGIN_PROC(printhex2)
		mov	r1, #2
printhex:	adr	r2, hexbuf
		add	r3, r2, r1
		mov	r1, #0
		strb	r1, [r3]
1:		and	r1, r0, #15
		mov	r0, r0, lsr #4
		cmp	r1, #10
		addlt	r1, r1, #'0'
		addge	r1, r1, #'a' - 10
		strb	r1, [r3, #-1]!
		teq	r3, r2
		bne	1b
		mov	r0, r2
		b	printascii

		.ltorg


	.align

	@ vmem-linked image has strings in vmem addresses. This replaces
	@ the reference with corresponding physical address. Note this
	@ won't work if memory offsets aren't clear cut values for
	@ orr'ing and bic'ing. rm = mmu bits rs = string address.
	.macro get_straddr rs, rm
		mrc	p15, 0, \rm, c1, c0		@ Get MMU bits.
		tst	\rm, #1				@ MMU enabled?
		biceq	\rs, \rs, #KERNEL_AREA_START	@ Clear Virtual mem offset.
		orreq	\rs, \rs, #PHYS_ADDR_BASE	@ Add Phy mem offset.
	.endm

BEGIN_PROC(printascii)
		get_straddr r0, r1
		uart_address r3
		b	2f
1:		uart_wait r2, r3
		uart_send r1, r3
		uart_busy r2, r3
		teq	r1, #'\n'
		moveq	r1, #'\r'
		beq	1b
2:		teq	r0, #0
		ldrneb	r1, [r0], #1
		teqne	r1, #0
		bne	1b
		mov	pc, lr
END_PROC(printascii)

BEGIN_PROC(printch)
		uart_address r3
		mov	r1, r0
		mov	r0, #0
		b	1b

hexbuf:		.space 16

