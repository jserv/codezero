/*
 * Simple linker script for userspace or svc tasks.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

virtual_base = 0x10000000;
/* __stack = (0x20000000 - 0x1000 - 8); */
/* First page before the env/args */


ENTRY(_start)

SECTIONS
{
	. = virtual_base;
	.text : { *(.text.head) *(.text) }
	.rodata : { *(.rodata) }
	.rodata1 : { *(.rodata1) }
	. = ALIGN(4K);
	.data :	{ *(.data) }
	.bss : { *(.bss) }
}
