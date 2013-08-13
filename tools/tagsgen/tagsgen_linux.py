#! /usr/bin/env python2.7
# -*- mode: python; coding: utf-8; -*-
#
# Call this script from codezero/conts/linux/kernel-2.6.34
#

import os, sys

PROJRELROOT = '../..'
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))

from scripts.config.configuration import *
config = configuration_retrieve()

platform = config.platform

def main():

    # Type of files to be included
    file_extn = ['*.c', '*.h', '*.s', '*.S', '*.lds', '*.in', 'Makefile', 'Kconfig']

    # Kernel directories
    src_dir_path = '.'      #'kernel-2.6.34'
    arch_dir_path = join(src_dir_path, 'arch/arm')

    # Add Makefiles etc present in base directories
    search_folder = [[src_dir_path, 1], [arch_dir_path, 1]]

    # Add architecture files
    search_folder += \
        [[join(arch_dir_path, 'boot'), 5],
         [join(arch_dir_path, 'common'), 5],
         [join(arch_dir_path, 'include'), 5],
         [join(arch_dir_path, 'kernel'), 5],
         [join(arch_dir_path, 'lib'), 5],
         [join(arch_dir_path, 'mm'), 5]]

    # Check for platform files
    if platform == 'pba9':
        search_folder += [[join(arch_dir_path, 'mach-vexpress'), 5],
                          [join(arch_dir_path, 'mach-versatile'), 5],
                          [join(arch_dir_path, 'plat-versatile'), 5]]
    elif platform == 'pb926':
        search_folder += [[join(arch_dir_path, 'mach-versatile'), 5],
                          [join(arch_dir_path, 'plat-versatile'), 5]]
    elif platform == 'eb':
        search_folder += [join(arch_dir_path, 'mach-realview'), 5]
    elif platform == 'beagle':
        search_folder += [[join(arch_dir_path, 'mach-omap2'), 5],
                          [join(arch_dir_path, 'plat-omap'), 5]]
                          # We may need mach-omap1 also.
    else:
        print 'Please define platform files path...'
        exit(1)


    # Add generic kernel directories
    search_folder += [[join(src_dir_path, 'mm'), 5],
                      [join(src_dir_path, 'lib'), 5],
                      [join(src_dir_path, 'kernel'), 5],
                      [join(src_dir_path, 'init'), 5],
                      [join(src_dir_path, 'include'), 5]]

    # Put all sources into a file list.
    for extn in file_extn:
        for dir, depth in search_folder:
            os.system("find " + dir + " -maxdepth " + str(depth) + \
                      " -name '" + extn + "' >> tagfilelist.linux")

    # Use file list to include in tags.
    os.system("ctags --languages=C,Asm --recurse -Ltagfilelist.linux --if0=yes")
    os.system("cscope -q -k -R -i tagfilelist.linux")

    # Delete generated files
    os.system("rm -f tagfilelist.linux")

if __name__ == "__main__":
    main()

