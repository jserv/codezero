/*
 *
 * Simple linker script
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */

/* FIXME:
 * Currently we can't include cpp #defines in linker script.
 * Check that below offsets are coherent with offsets.h
 */
phys_addr_base		= 0x100000;
kern_offset		= 0xF0000000;
virt_addr_base		= phys_addr_base + kern_offset;

/* A temporary boot stack is used before a proper kernel stack is set up */
_bootstack_physical	= _bootstack - kern_offset;

/* The symbols are linked at virtual addresses. So is _start.
 * We must set the entry point to a physical address, so that
 * when the image is loaded, it doesn't jump to a non existing
 * virtual address.
 */
_start_physical		= phys_addr_base;

ENTRY(_start_physical)

SECTIONS
{
	. = virt_addr_base;
	_start_kernel = .;
	.text : AT (ADDR(.text) - kern_offset)
	{
		_start_text = .;
		/* Make sure head.S comes first */
		/* *head.o(.text) This only works when given its full path. Bad limitation. */
		*(.text.head)
		*(.text)
		_end_text = .;
	}
	. = ALIGN(4);
	/* rodata is needed else your strings will link at physical! */
	.rodata : AT (ADDR(.rodata) - kern_offset) { *(.rodata) }
	.rodata1 : AT (ADDR(.rodata1) - kern_offset) { *(.rodata1) }
	.data : AT (ADDR(.data) - kern_offset)
	{
		_start_data = .;
		*(.data)
		_start_vectors = .;
		*(.data.vectors)
		. = ALIGN(4K);
		_end_vectors = .;
		_start_kip = .;
		*(.data.kip)
		. = ALIGN(4K);
		_end_kip = .;
		_start_syscalls = .;
		*(.data.syscalls)
		. = ALIGN(4K);
		_end_syscalls = .;
		_end_data = .;
	}
	.bss : AT (ADDR(.bss) - kern_offset)
	{
		*(.bss)
	}
	. = ALIGN(4K);
	. += 0x2000;		/* This is required as the link counter does not seem
				 * to increment for the bss section
				 * TODO: Change this with PAGE_SIZE */
	_bootstack = .;
	_end_kernel = .;
	. = ALIGN(1M);
	.kspace : AT(ADDR(.kspace) - kern_offset)
	{
		_start_kspace = .;
		*(.kspace.pgd)
		. = ALIGN(4K);
		_start_pmd = .;
		*(.kspace.pmd)
		_end_pmd = .;
		_end_kspace = .;
	}
	_end = .;
}

