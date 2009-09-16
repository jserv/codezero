/*
 * Linker script that packs all containers in loader image.
 *
 * Copyright (C) 2007-2009 B Labs Ltd.
 */
ENTRY(_start)

SECTIONS
{
	. = 0x2000000;
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.rodata1 : { *(.rodata1) }
	.data :
	{
        *(.data)
		_start_kernel = .;
		*(.kernel)
		_end_kernel = .;
	}
	_start_containers = .;

	.cont.0 : { *(.cont.0) }
	.cont.1 : { *(.cont.1) }

	_end_containers = .;
	.got : { *(.got) *(.got.plt) }
	.bss : { *(.bss) }
}
