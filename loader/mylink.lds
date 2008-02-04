/*
 * Simple linker script for userspace or svc tasks.
 *
 * Copyright (C) 2007 Bahadir Balban
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
		_start_bootdesc = .;
		*(.bootdesc)
		_end_bootdesc = .;
		_start_mm0 = .;
		*(.mm0)
		_end_mm0 = .;
		_start_fs0 = .;
		*(.fs0)
		_end_fs0 = .;
		_start_test0 = .;
		*(.test0)
		_end_test0 = .;
		*(.data)
	}
	.got : { *(.got) *(.got.plt) }
	.bss : { *(.bss) }
}
