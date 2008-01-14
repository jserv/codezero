#!/usr/bin/env python

from aistruct import AIStruct
import elf, sys
from optparse import OptionParser
from os import path

class AfterBurner(AIStruct):
	def __init__(self, *args, **kwargs):
		AIStruct.__init__(self, AIStruct.SIZE32)
		self.setup(
			('UINT32', 'addr')
		)

        def __str__(self):
            return "0x%x" % self.ai.addr.get()

def main():
    parser = OptionParser(add_help_option=False)
    parser.add_option("-h", "--file-header",
                      action="store_true", dest="header", default=False,
                      help="Display the ELF file header")
    parser.add_option("-l", "--program-headers",
                      action="store_true", dest="program_headers", default=False,
                      help="Display the program headers")
    parser.add_option("-S", "--section-headers",
                      action="store_true", dest="section_headers", default=False,
                      help="Display the section headers")
    parser.add_option("--afterburn",
                      action="store_true", dest="afterburn", default=False,
                      help="Display the afterburn relocations")
    parser.add_option("--first-free-page", 
		      action="store_true", dest="ffpage", default=False,
		      help="Prints out (in .lds format) the address of the first free physical" + \
		      	   "page after this image at load time. Using this information at link" + \
			   "time, images can be compiled and linked consecutively and loaded in" + \
		      	   "consecutive memory regions at load time.")
    parser.add_option("--lma-start-end", action="store_true", dest="lma_boundary", default=False,
		      help="Prints out the start and end LMA boundaries of an image." + \
		      	   "This is useful for autogenerating a structure for the microkernel" + \
			   "to discover at run-time where svc tasks are loaded.")
    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.print_help()
        return
    elffile = elf.ElfFile.from_file(args[0])

    if options.header:
        print elffile.header
    if options.program_headers:
        print elffile.pheaders
    if options.section_headers:
        print elffile.sheaders
    if options.afterburn:
        burnheader = elffile.sheaders[".afterburn"]
        burns = burnheader.container(AfterBurner)
        print "There are %d afterburn entry points" % len(burns)
        print "Afterburn:"
        for burn in burns:
            print " ", burn
    if options.lma_boundary:
       paddr_first = 0
       paddr_start = 0
       paddr_end = 0
       for pheader in elffile.pheaders:
           x = pheader.ai
	   if str(x.p_type) != "LOAD":
	   	continue
           # First time assign the first p_paddr.
	   if paddr_first == 0:
	       paddr_first = 1
	       paddr_start = x.p_paddr.value
	   # Then if new paddr is lesser, reassign start.
	   if paddr_start > x.p_paddr.value:
	   	paddr_start = x.p_paddr.value
           # If end of segment is greater, reassign end.
	   if paddr_end < x.p_paddr + x.p_memsz:
	   	paddr_end = x.p_paddr + x.p_memsz
       rest, image_name = path.split(args[0])
       if image_name[-4] == ".":
           image_name = image_name[:-4]
       print image_name
       if hex(paddr_start)[-1] == "L":
           print "image_start " + hex(paddr_start)[:-1]
       else:
       	    print "image_start " + hex(paddr_start)
       if hex(paddr_end)[-1] == "L":
            print "image_end " + hex(paddr_end)[:-1]
       else:
            print "image_end " + hex(paddr_end)

    if options.ffpage:
        paddr_max = 0
	p_align = 0
        for pheader in elffile.pheaders:
        	x = pheader.ai
		if str(x.p_type) == "LOAD":
			paddr = x.p_paddr + x.p_memsz
			p_align = x.p_align
			if paddr > paddr_max:
				paddr_max = paddr

	print "/*\n" + \
	      " * The next free p_align'ed LMA base address\n" + \
	      " *\n" + \
	      " * p_align = " + hex(p_align.value) + "\n" + \
	      " *\n" + \
	      " * Recap from ELF spec: p_align: Loadable process segments must have\n" + \
	      " * congruent values for p_vaddr and p_offset, modulo the page size. \n" + \
	      " * This member gives the value to which the segments are aligned in \n" + \
	      " * memory and in the file. Values 0 and 1 mean that no alignment is \n" + \
	      " * required. Otherwise, p_align should be a positive, integral power\n" + \
	      " * of 2, and p_addr should equal p_offset, modulo p_align. \n" + \
	      " * This essentially means next available address must be aligned at\n" + \
	      " * p_align, rather than the page_size, which one (well, I) would \n" + \
	      " * normally expect. \n" + \
	      " */\n"
	paddr_aligned = paddr_max & ~(p_align.value - 1)
	if paddr_max & (p_align.value - 1):
		paddr_aligned += p_align.value
	if hex(paddr_aligned)[-1] == "L":
		print "physical_base = " + hex(paddr_aligned)[:-1] + ";"
	else:
		print "physical_base = " + hex(paddr_aligned) + ";"


if __name__ == "__main__":
    main()
