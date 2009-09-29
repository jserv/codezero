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
/* USER_AREA_START, see memlayout.h */
virtual_base = 0x10000000;
physical_base = 0x8000;
__stack = (0x20000000 - 0x1000 - 8); /* First page before the env/args */
/* INCLUDE "include/physical_base.lds" */

/* physical_base = 0x228000; */
offset = virtual_base - physical_base;

ENTRY(_start)

SECTIONS
{
	. = virtual_base;
	_start_text = .;
	.text : AT (ADDR(.text) - offset) { *(.text.head) *(.text) }
	/* rodata is needed else your strings will link at physical! */
	.rodata : AT (ADDR(.rodata) - offset) { *(.rodata) }
	.rodata1 : AT (ADDR(.rodata1) - offset) { *(.rodata1) }
	.data : AT (ADDR(.data) - offset)
	{
		. = ALIGN(4K);
		_start_test_exec = .;
		*(.testexec)
		_end_test_exec = .;
		*(.data)
	}
	.bss : AT (ADDR(.bss) - offset) { *(.bss) }
	_end = .;
}
