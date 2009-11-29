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
                            ['PB1176', '0x5E0'],
                            ['EB', '0x33B'],
                            ['PB11MPCORE', '0x3D4'],)

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
    def modify_register_values(self, config, container):
        for cpu_type, cpu_id in self.cpuid_list:
            if cpu_type == config.cpu.upper():
                cpuid = cpu_id
                break
        for arch_type, arch_id in self.archid_list:
            if arch_type == config.platform.upper():
                archid = arch_id
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
        for param_name, param_value in self.config_param_list:
            param = 'CONFIG_' + param_name
            prev_line = ''
            if param_value == 'SET':
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

        self.container = container
        self.kernel_binary_image = \
            join(os.path.relpath(self.LINUX_KERNEL_BUILDDIR, LINUX_KERNELDIR), \
                 "vmlinux")
        self.kernel_image = join(self.LINUX_KERNEL_BUILDDIR, "linux.elf")
        self.kernel_updater = LinuxUpdateKernel(self.container)

    def build_linux(self, config):
        print '\nBuilding the linux kernel...'
        os.chdir(self.LINUX_KERNELDIR)
        if not os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            os.makedirs(self.LINUX_KERNEL_BUILDDIR)

        self.kernel_updater.modify_kernel_config()
        self.kernel_updater.update_kernel_params(self.container)
        self.kernel_updater.modify_register_values(config, self.container)

        os.system("make defconfig ARCH=arm O=" + self.LINUX_KERNEL_BUILDDIR)
        os.system("make ARCH=arm " + \
                  "CROSS_COMPILE=" + config.user_toolchain + " O=" + \
                  self.LINUX_KERNEL_BUILDDIR)

        # Generate kernel_image, elf to be used by codezero
        linux_elf_gen_cmd = ("arm-none-linux-gnueabi-objcopy -R .note \
            -R .note.gnu.build-id -R .comment -S --change-addresses " + \
            str(conv_hex(-self.container.linux_page_offset + self.container.linux_phys_offset)) + \
            " " + self.kernel_binary_image + " " + self.kernel_image)

        #print cmd
        os.system(linux_elf_gen_cmd)
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
