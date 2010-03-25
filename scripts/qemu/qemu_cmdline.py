#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- Virtualization microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys
from os.path import join

PROJRELROOT = "../.."
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))

from config.projpaths import *
from config.configuration import *

#config = configuration_retrieve()
#cpu = config.cpu
#platform = config.platform

# Mapping between system configuration and qemu flags
#           Platform         CPU           qemu "-M" flag          qemu "-cpu" flag
map_list = (['EB',          'ARM1136',      'realview-eb',          'arm1136'],
            ['EB',          'ARM11MPCORE',  'realview-eb-mpcore',   'arm11mpcore'],
            ['EB',          'CORTEXA8',     'realview-eb',          'cortex-a8'],
            ['PB926',       'ARM926',       'versatilepb',          'arm926'],
            ['BEAGLE',      'CORTEXA8',     'beagle',               'cortex-a8'],
            ['PBA9',        'CORTEXA9',     'realview-pbx-a9',      'cortex-a9'],
            ['PBA8',        'CORTEXA8',     'realview-pb-a8',       'cortex-a8'])

data = \
'''
cd build
qemu-system-arm -s -S -kernel final.elf -nographic -M %s -cpu %s &
arm-none-insight ; pkill qemu-system-arm
cd ..
'''

def build_qemu_cmdline_script():
    build_tools_folder = 'tools'
    qemu_cmd_file = join(build_tools_folder, 'run-qemu-insight')

    # Get system selected platform and cpu
    config = configuration_retrieve()
    cpu = config.cpu.upper()
    platform = config.platform.upper()

    # Find appropriate flags
    for platform_type, cpu_type, mflag, cpuflag in map_list:
        if platform_type == platform and cpu_type == cpu:
            break

    if not mflag or not cpuflag:
        print 'Qemu flags not found'
        sys.exit(1)

    if os.path.exists(build_tools_folder) is False:
        os.system("mkdir " + build_tools_folder)

    # Write run-qemu-insight file
    with open(qemu_cmd_file, 'w+') as f:
        f.write(data % (mflag, cpuflag))

    os.system("chmod +x " + qemu_cmd_file)

if __name__ == "__main__":
    build_qemu_cmdline_script()
