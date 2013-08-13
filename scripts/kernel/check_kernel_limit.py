#! /usr/bin/env python2.7
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys
from tools.pyelf.elfsize import *

from scripts.config.projpaths import *
from scripts.config.configuration import *

def get_kernel_end_address(img):
    kernel_size = elf_binary_size(img)
    kernel_start = get_elf_load_address(img)
    return (int(kernel_start.get()) + kernel_size)

def get_container_start():
    start = 0xffffffff
    with open(join(PROJROOT, CONFIG_H), 'r')as file:
        for line in file:
            begin = line.rfind(" ")
            end = len(line)
            if re.search("(PHYS)([0-9]){1,4}(_START)", line) and \
               start > int(line[begin : end], 16):
                   start = int(line[begin : end], 16)
    return start

def check_kernel_container_overlap():
    kernel_end = get_kernel_end_address(KERNEL_ELF)
    cont_start = get_container_start()
    if kernel_end > cont_start:
        print '\nKernel end address = ' + str(hex(kernel_end))
        print 'Container start address = ' + str(hex(cont_start)) + '\n'
        return 1
    return 0

if __name__ == "__main__":
        check_kernel_container_overlap()
