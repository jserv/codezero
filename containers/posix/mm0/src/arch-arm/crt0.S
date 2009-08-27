
.section .text.head
.global _start;
.type	_start,function;
.align;

_start:
	ldr	sp, =__stack
	bl	__container_init
1:
	b	1b

