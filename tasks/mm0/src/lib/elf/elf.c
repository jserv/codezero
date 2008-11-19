/*
 * ELF manipulation routines
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <vm_area.h>
#include <lib/elf.h>
#include <lib/elfprg.h>
#include <lib/elfsym.h>
#include <lib/elfsect.h>


int elf_probe(struct elf_header *header)
{
	/* Test that it is a 32-bit little-endian ELF file */
	if (header->e_ident[EI_MAG0] == ELFMAG0 &&
	    header->e_ident[EI_MAG1] == ELFMAG1 &&	
	    header->e_ident[EI_MAG2] == ELFMAG2 &&	
	    header->e_ident[EI_MAG3] == ELFMAG3 &&	
	    header->e_ident[EI_CLASS] == ELFCLASS32 &&
	    header->e_ident[EI_DATA] == ELFDATA2LSB)
		return 0;
	else
		return -1;
}


int elf_parse_executable(struct vm_file *f)
{
	int err;
	struct elf_header *elf_header = pager_map_page(f, 0);
	struct elf_program_header *prg_header;
	struct elf_section_header *sect_header;
		
	/* Test that it is a valid elf file */
	if ((err = elf_probe(elf_header)) < 0)
		return err;

	/* Get the program header table */
	prg_header = (struct elf_program_header *)((void *)elf_header + elf_header->e_phoff);
}

