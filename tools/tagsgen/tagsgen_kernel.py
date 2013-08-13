#! /usr/bin/env python2.7
# -*- mode: python; coding: utf-8; -*-

import os, sys

PROJRELROOT = '../../'
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))

from scripts.config.configuration import *
config = configuration_retrieve()

platform = config.platform
subarch = config.subarch
arch = config.arch

def main():
    os.system("rm -f cscope.*")
    os.system("rm -f tags")

    # Type of files to be included
    file_extn = ['*.cc', '*.c', '*.h', '*.s', '*.S', '*.lds']

    # Kernel directories
    src_dir_path = 'src'
    include_dir_path = 'include/l4'

    search_folder = \
        ['api',
         'arch/'+ arch,
         'arch/'+ arch + '/' + subarch,
         'generic',
         'glue/' + arch,
         'lib',
         'platform/' + platform]

    # Check if we need realview directory based on platform selected
    if platform == 'eb' or platform == 'pba9':
        search_folder += ['platform/realview']

    # Driver selection based on platform selected
    if platform == 'beagle':
        search_folder += ['drivers/uart/omap', 'drivers/timer/omap', 'drivers/irq/omap3']
    elif platform == 'eb' or platform == 'pba9':
        search_folder += ['drivers/uart/pl011', 'drivers/timer/sp804', 'drivers/irq/gic']
    elif platform == 'pb926':
        search_folder += ['drivers/uart/pl011', 'drivers/timer/sp804', 'drivers/irq/pl190']

    # Put all sources into a file list.
    depth = 1
    for extn in file_extn:
        for dir in search_folder:
            os.system("find " + join(src_dir_path, dir) + \
                " -maxdepth " + str(depth) + " -name '" + extn + "' >> tagfilelist")
            os.system("find " + join(include_dir_path, dir) + \
                " -maxdepth " + str(depth) + " -name '" + extn + "' >> tagfilelist")

    # Use file list to include in tags.
    os.system("ctags --languages=C,Asm --recurse -Ltagfilelist --if0=yes")
    os.system("cscope -q -k -R -i tagfilelist")

    # Remove file list.
    os.system("rm -f tagfilelist")

if __name__ == "__main__":
    main()

