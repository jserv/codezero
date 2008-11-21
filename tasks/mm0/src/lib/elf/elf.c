/*
 * ELF manipulation routines
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <vm_area.h>
#include <lib/elf/elf.h>
#include <lib/elf/elfprg.h>
#include <lib/elf/elfsym.h>
#include <lib/elf/elfsect.h>


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

static inline void *pager_map_file_offset(struct vm_file *f, unsigned long f_offset)
{
	void *page = pager_map_page(f, __pfn(f_offset));

	return (void *)((unsigned long)page | (PAGE_MASK & f_offset));
}

/*
 * Loading an ELF file:
 *
 * This first probes and detects that the given file is a valid elf file.
 * Then it looks at the program header table to find the first (probably
 * only) segment that has type LOAD. Then it looks at the section header
 * table, to find out about every loadable section that is part of this
 * aforementioned loadable program segment. Each section is marked in the
 * efd and tcb structures for further memory mappings.
 */
int elf_parse_executable(struct tcb *task, struct vm_file *file,
			 struct exec_file_desc *efd)
{
	int err;
	struct elf_header *elf_header = pager_map_page(file, 0);
	struct elf_program_header *prg_header_start, *prg_header_load;
	struct elf_section_header *sect_header;

	/* Test that it is a valid elf file */
	if ((err = elf_probe(elf_header)) < 0)
		return err;

	/* Get the program header table */
	prg_header_start = (struct elf_program_header *)
			   ((void *)elf_header + elf_header->e_phoff);

	/* Get the first loadable segment */
	for (int i = 0; i < elf_header->e_phnum; i++) {
		if (prg_header_start[i].p_type == PT_LOAD) {
			prg_header_load = &prg_header_start[i];
			break;
		}
	}

	/* Get the section header table */
	if (__pfn(elf_header->e_shoff) > 0)
		sect_header = pager_map_file_offset(file, elf_header->e_shoff);
	else
		sect_header = (struct elf_section_header *)
			      ((void *)elf_header + elf_header->e_shoff);

	/* Determine if we cross a page boundary during traversal */
	if (elf_header->e_shentsize * elf_header->e_shnum > TILL_PAGE_ENDS(sect_header))

	/*
	 * Sift through sections and copy their marks to tcb and efd
	 * if they are recognised and loadable sections.
	 *
	 * NOTE: There may be multiple sections of same kind, in
	 * consecutive address regions. Then we need to increase
	 * that region's marks.
	 */
	for (int i = 0; i < elf_header->e_shnum; i++) {
		struct elf_section_header *section = &sect_header[i];

		/* Text section */
		if (section->sh_type == SHT_PROGBITS &&
		    section->sh_flags & SHF_ALLOC &&
		    section->sh_flags & SHF_EXECINSTR) {
			efd->text_offset = section->sh_offset;
			task->text_start = section->sh_addr;
			task->text_end = section->sh_addr + section->sh_size;
		}

		/* Data section */
		if (section->sh_type == SHT_PROGBITS &&
		    section->sh_flags & SHF_ALLOC &&
		    section->sh_flags & SHF_WRITE) {
			efd->data_offset = section->sh_offset;
			task->data_start = section->sh_addr;
			task->data_end = section->sh_addr + section->sh_size;
		}

		/* BSS section */
		if (section->sh_type == SHT_NOBITS &&
		    section->sh_flags & SHF_ALLOC &&
		    section->sh_flags & SHF_WRITE) {
			efd->bss_offset = section->sh_offset;
			task->bss_start = section->sh_addr;
			task->bss_end = section->sh_addr + section->sh_size;
		}
	}

	return 0;
}

