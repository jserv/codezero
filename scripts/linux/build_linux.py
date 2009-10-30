#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys, shelve, string
from os.path import join

PROJRELROOT = '../../'

SCRIPTROOT = os.path.abspath(os.path.dirname("."))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))

from config.projpaths import *
from config.configuration import *
from config.lib import *

LINUX_KERNEL_BUILDDIR = join(BUILDDIR, os.path.relpath(LINUX_KERNELDIR, PROJROOT))

# Create linux kernel build directory path as:
# conts/linux -> build/cont[0-9]/linux
def source_to_builddir(srcdir, id):
    cont_builddir = \
        os.path.relpath(srcdir, \
                        PROJROOT).replace("conts", \
                                          "cont" + str(id))
    return join(BUILDDIR, cont_builddir)

class LinuxUpdateKernel:

    def __init__(self, container):
        # List for setting/unsetting .config params of linux
        self.config_param_list = \
            (['PCI', 'SET'],['AEABI', 'SET'],
            ['SCSI', 'SET'],['BLK_DEV_SD', 'SET'],
            ['SYM53C8XX_2', 'SET'],['INPUT_EVDEV', 'SET'],
            ['INOTIFY', 'SET'],['DEBUG_INFO', 'SET'],
            ['USB_SUPPORT', 'UNSET'],['SOUND', 'UNSET'],)

        # List of CPUIDs, to be used by linux based on codezero config
        self.cpuid_list = (['ARM926', '0x41069265'],)
        # List of ARCHIDs, to be used by linux based on codezero config
        self.archid_list = (['PB926', '0x183'],
                            ['AB926', '0x25E'],
                            ['PB1176', '0x5E0'],
                            ['REALVIEW_EB', '33B'],)

    # Replace line(having input_pattern) in filename with new_data
    def replace_line(self, filename, input_pattern, new_data, prev_line):
        with open(filename, 'r+') as f:
            flag = 0
            temp = 0
            x = re.compile(input_pattern)
            for line in f:
                if '' != prev_line:
                    if temp == prev_line and re.match(x, line):
                        flag = 1
                        break
                    temp = line
                else:
                    if re.match(x, line):
                        flag = 1
                        break

            if flag == 0:
                #print 'Warning: No match found for the parameter'
                return
            else:
                # Prevent recompilation in case kernel parameter is same
                if new_data != line:
                    f.seek(0)
                    l = f.read()

                    # Need to truncate file because, size of contents to be
                    # written may be less than the size of original file.
                    f.seek(0)
                    f.truncate(0)

                    # Write back to file
                    f.write(l.replace(line, new_data))

    def update_kernel_params(self, container):
        # Update TEXT_START
        file = join(LINUX_KERNELDIR, 'arch/arm/boot/compressed/Makefile')
        param = str(conv_hex(container.linux_phys_offset))
        new_data = ('ZTEXTADDR' + '\t' + ':= ' + param + '\n')
        data_to_replace = "(ZTEXTADDR)(\t)(:= 0)"
        prev_line = ''
        self.replace_line(file, data_to_replace, new_data, prev_line)

        # Update PHYS_OFFSET
        file = join(LINUX_KERNELDIR, 'arch/arm/mach-versatile/include/mach/memory.h')
        param = str(conv_hex(container.linux_phys_offset))
        new_data = ('#define PHYS_OFFSET     UL(' + param + ')\n')
        data_to_replace = "(#define PHYS_OFFSET)"
        prev_line = ''
        self.replace_line(file, data_to_replace, new_data, prev_line)

        # Update PAGE_OFFSET
        file = join(LINUX_KERNELDIR, 'arch/arm/Kconfig')
        param = str(conv_hex(container.linux_page_offset))
        new_data = ('\t' + 'default ' + param + '\n')
        data_to_replace = "(\t)(default )"
        prev_line = ('\t'+'default 0x80000000 if VMSPLIT_2G' + '\n')
        self.replace_line(file, data_to_replace, new_data, prev_line)

        # Update ZRELADDR
        file = join(LINUX_KERNELDIR, 'arch/arm/mach-versatile/Makefile.boot')
        param = str(conv_hex(container.linux_zreladdr))
        new_data = ('   zreladdr-y' + '\t' + ':= ' + param + '\n')
        data_to_replace = "(\s){3}(zreladdr-y)(\t)(:= )"
        prev_line = ''
        self.replace_line(file, data_to_replace, new_data, prev_line)

    # Update ARCHID, CPUID and ATAGS ADDRESS
    def modify_register_values(self, container):
        # Patterns as defined in config.h
        cpuid_pattern = '#define CONFIG_CPU_'
        archid_pattern = '#define CONFIG_PLATFORM_'

        config_h_path = join(PROJROOT, CONFIG_H)

        for pattern in cpuid_pattern, archid_pattern:
            with open(config_h_path, 'r') as f:
                for line in f:
                    start = string.find(line, pattern)
                    if start == -1:
                        continue
                    else:
                        end = start + len(pattern)
                        start = end
                        while line[end] != ' ':
                            end = end + 1
                        if pattern == cpuid_pattern:
                            cpu_type = line[start:end]
                        elif pattern == archid_pattern:
                            arch_type = line[start:end]
                        break
        for i in self.cpuid_list:
            if i[0] == cpu_type:
                cpuid = i[1]
                break
        for i in self.archid_list:
            if i[0] == arch_type:
                archid = i[1]
                break

        file = join(LINUX_KERNELDIR, 'arch/arm/kernel/head.S')
        prev_line = ''
        new_data = ('cpuid:  .word   ' + cpuid + '\n')
        data_to_replace = "(cpuid:)"
        self.replace_line(file, data_to_replace, new_data, prev_line)

        new_data = ('archid: .word   ' + archid + '\n')
        data_to_replace = "(archid:)"
        self.replace_line(file, data_to_replace, new_data, prev_line)
        # Atags will be present at PHYS_OFFSET + 0x100(=256)
        new_data = ('atags:  .word   ' + \
                    str(conv_hex(container.linux_phys_offset + 0x100)) + '\n')
        data_to_replace = "(atags:)"
        self.replace_line(file, data_to_replace, new_data, prev_line)

    def modify_kernel_config(self):
        file = join(LINUX_KERNELDIR, 'arch/arm/configs/versatile_defconfig')
        for i in self.config_param_list:
            param = 'CONFIG_' + i[0]
            prev_line = ''
            if i[1] == 'SET':
                data_to_replace = ('# ' + param)
                new_data = (param + '=y' + '\n')
            else:
                data_to_replace = param
                new_data = ('# ' + param + ' is not set' + '\n')

            self.replace_line(file, data_to_replace, new_data, prev_line)

class LinuxBuilder:

    def __init__(self, pathdict, container):
        self.LINUX_KERNELDIR = pathdict["LINUX_KERNELDIR"]

        # Calculate linux kernel build directory
        self.LINUX_KERNEL_BUILDDIR = \
            source_to_builddir(LINUX_KERNELDIR, container.id)

        self.linux_lds_in = join(self.LINUX_KERNELDIR, "linux.lds.in")
        self.linux_lds_out = join(self.LINUX_KERNEL_BUILDDIR, "linux.lds")
        self.linux_S_in = join(self.LINUX_KERNELDIR, "linux.S.in")
        self.linux_S_out = join(self.LINUX_KERNEL_BUILDDIR, "linux.S")

        self.linux_h_in = join(self.LINUX_KERNELDIR, "linux.h.in")
        self.linux_h_out = join(self.LINUX_KERNEL_BUILDDIR, "linux.h")

        self.linux_elf_out = join(self.LINUX_KERNEL_BUILDDIR, "linux.elf")

        self.container = container
        self.kernel_binary_image = \
            join(os.path.relpath(self.LINUX_KERNEL_BUILDDIR, LINUX_KERNELDIR), \
                 "arch/arm/boot/Image")
        self.kernel_image = None
        self.kernel_updater = LinuxUpdateKernel(self.container)

    def build_linux(self):
        print '\nBuilding the linux kernel...'
        # TODO: Need to sort this, we cannot call it in global space
        # as configuration file is not presnt in beginning
        config = configuration_retrieve()

        os.chdir(self.LINUX_KERNELDIR)
        if not os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            os.makedirs(self.LINUX_KERNEL_BUILDDIR)

        self.kernel_updater.modify_kernel_config()
        self.kernel_updater.update_kernel_params(self.container)
        self.kernel_updater.modify_register_values(self.container)

        os.system("make defconfig ARCH=arm O=" + self.LINUX_KERNEL_BUILDDIR)
        os.system("make ARCH=arm " + \
                  "CROSS_COMPILE=" + config.user_toolchain + " O=" + \
                  self.LINUX_KERNEL_BUILDDIR)

        with open(self.linux_h_out, 'w+') as output:
            with open(self.linux_h_in, 'r') as input:
                output.write(input.read() % {'cn' : self.container.id})

        with open(self.linux_S_in, 'r') as input:
            with open(self.linux_S_out, 'w+') as output:
                content = input.read() % self.kernel_binary_image
                output.write(content)

        os.system(config.user_toolchain + "cpp -I%s -P %s > %s" % \
                  (self.LINUX_KERNEL_BUILDDIR, self.linux_lds_in, \
                   self.linux_lds_out))
        os.system(config.user_toolchain + "gcc -nostdlib -o %s -T%s %s" % \
                  (self.linux_elf_out, self.linux_lds_out, self.linux_S_out))

        # Get the kernel image path
        self.kernel_image = self.linux_elf_out

        print 'Done...'

    def clean(self):
        print 'Cleaning linux kernel build...'
        if os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            shutil.rmtree(self.LINUX_KERNEL_BUILDDIR)
        print 'Done...'

if __name__ == "__main__":
    # This is only a default test case
    container = Container()
    container.id = 0
    linux_builder = LinuxBuilder(projpaths, container)

    if len(sys.argv) == 1:
        linux_builder.build_linux()
    elif "clean" == sys.argv[1]:
        linux_builder.clean()
    else:
        print " Usage: %s [clean]" % (sys.argv[0])
