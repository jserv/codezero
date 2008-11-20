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

/*
 * Loading an ELF file:
 *
 * This first probes and detects that the given file is a valid elf file.
 * Then it looks at the program header table to find the first (probably
 * only) segment that has type LOAD. Then it looks at the section header
 * table, to find out about every loadable section that is part of this
 * aforementioned loadable program segment. Each section is marked in the
 * efd and tcb structures for further memory mappings. Loading an elf
 * executable is simple as that, but it is described poorly in manuals.
 */
int elf_parse_executable(struct tcb *task, struct vm_file *file,
			 struct exec_file_desc *efd)
{
	int err;
	struct elf_header *elf_header = pager_map_page(file, 0);
	struct elf_program_header *prg_header_start, *prg_header_load;
	struct elf_section_header *sect_header_start;

	/* Test that it is a valid elf file */
	if ((err = elf_probe(elf_header)) < 0)
		return err;

	/* Get the program header table */
	prg_header_start = (struct elf_program_header *)
			   ((void *)elf_header + elf_header->e_phoff);

	/* Get the first loadable segment */
	for (int i = 0; i < elf_header->e_phnum; i++) {
		if (prg_header_start[i].type == PT_LOAD) {
			prg_header_load = &prg_header_start[i];
			break;
		}
	}

	/* Get the section header table */
	sect_header_start = (struct elf_section_header *)
			    ((void *)elf_header + elf_header->e_shoff);
}

