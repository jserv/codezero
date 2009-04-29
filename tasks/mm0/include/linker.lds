/*
 * Simple linker script for userspace or svc tasks.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

/*
 * The only catch with this linker script is that everything
 * is linked starting at virtual_base, and loaded starting
 * at physical_base. virtual_base is the predefined region
 * of virtual memory for userland applications. physical_base
 * is determined at build-time, it is one of the subsequent pages
 * that come after the kernel image's load area.
 */
/* INITTASK_AREA_START, see memlayout.h */
virtual_base = 0xE0000000;
INCLUDE "include/physical_base.lds"

/* physical_base = 0x228000; */
offset = virtual_base - physical_base;

ENTRY(_start)

SECTIONS
{
	. = virtual_base;
	_start_text = .;
	.text : AT (ADDR(.text) - offset) { crt0.o(.text) *(.text) }
	/* rodata is needed else your strings will link at physical! */
	.rodata : AT (ADDR(.rodata) - offset) { *(.rodata) }
	.rodata1 : AT (ADDR(.rodata1) - offset) { *(.rodata1) }
	.data : AT (ADDR(.data) - offset)
	{
		*(.data)
	}
	. = ALIGN(4K);
	_start_init = .;
	.init : AT (ADDR(.init) - offset) { *(.init.stack) }
	. = ALIGN(8);
	__stack = .;	/* This is the preallocated boot stack */
	_end_init = .;
	.bss : AT (ADDR(.bss) - offset) { *(.bss) }
	_end = .;
}
