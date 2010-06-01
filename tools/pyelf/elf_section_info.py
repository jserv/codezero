#!/usr/bin/env python

import elf

# Define header markers for various program headers in elf
def elf_loadable_section_info(img):
    elffile = elf.ElfFile.from_file(img)

    # Markers
    rw_sections_start = 0
    rw_sections_end = 0
    rx_sections_start = 0
    rx_sections_end = 0

    # Flag encoding used by elf
    PF_X = 1 << 0
    PF_W = 1 << 1
    PF_R = 1 << 2

    loadable_header = 1

    for pheader in elffile.pheaders:
	x = pheader.ai

        # Check for loadable headers
	if x.p_type.get() >> loadable_header == 0:
		start = x.p_vaddr.get()
		end = start + x.p_memsz.get()

		# RW header
		if x.p_flags.get() & (~(PF_R | PF_W)) == 0:
			if (rw_sections_start == 0) or (rw_sections_start > start):
				rw_sections_start = start
			if (rw_sections_end == 0) or (rw_sections_end < end):
				rw_sections_end = end
            	# RX header
		elif x.p_flags.get() & (~(PF_R | PF_X)) == 0:
		    if (rx_sections_start == 0) or (rx_sections_start > start):
			    rx_sections_start = start
                    if (rx_sections_end == 0) or (rx_sections_end < end):
                            rx_sections_end = end

    return rw_sections_start, rw_sections_end, \
           rx_sections_start, rx_sections_end
