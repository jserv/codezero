/*
 * Loader linker script that contains kernel and container images
 *
 * Copyright (C) 2008-2009 B Labs Ltd.
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
		_start_kernel = .;
		*(.kernel)
		_end_kernel = .;

		_start_containers = .;
		*(.containers)
		_end_containers = .;
        	*(.data)
	}
	.got : { *(.got) *(.got.plt) }
	.bss : { *(.bss) }
}
