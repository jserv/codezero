#! /usr/bin/env python2.7
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys, shelve, string
from os.path import join

PROJRELROOT = '../../'
SCRIPTROOT = os.path.abspath(os.path.dirname('.'))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))

from scripts.config.projpaths import *
from scripts.config.configuration import *
from scripts.config.lib import *

LINUX_KERNEL_BUILDDIR = join(BUILDDIR, os.path.relpath(LINUX_KERNELDIR, PROJROOT))

# Create linux kernel build directory path as:
# conts/linux -> build/cont[0-9]/linux
def source_to_builddir(srcdir, id):
    cont_builddir = \
        os.path.relpath(srcdir, PROJROOT).replace('conts', 'cont' + str(id))
    return join(BUILDDIR, cont_builddir)

class LinuxUpdateKernel:
    def __init__(self, container):
        # List of CPUIDs, to be used by linux based on codezero config
        self.cpuid_list = {'ARM926'     :   '0x41069265',
                           'CORTEXA8'   :   '0x410fc080',
                           'ARM11MPCORE':   '0x410fb022',
                           'CORTEXA9'   :   '0x410fc090'}

        # List of ARCHIDs, to be used by linux based on codezero config
        self.archid_list = {'PB926' :   '0x183',
                            'EB'    :   '0x33B',
                            'BEAGLE':   '0x60A',
                            'PBA9'  :   '0x76D'}

        # Path of system_macros header file
        self.system_macros_h_out = join(LINUX_KERNELDIR, 'arch/arm/include/vmm/system_macros.h')
        self.system_macros_h_in = join(LINUX_KERNELDIR, 'arch/arm/include/vmm/system_macros.h.in')

        self.kconfig_in = join(LINUX_KERNELDIR, 'arch/arm/Kconfig.in')
        self.kconfig_out = join(LINUX_KERNELDIR, 'arch/arm/Kconfig')

    # Update kernel parameters
    def update_kernel_params(self, config, container):
        with open(self.kconfig_out, 'w+') as output:
            with open(self.kconfig_in, 'r') as input:
                output.write(input.read() %  \
                    {'phys_offset'  : str(conv_hex(container.linux_phys_offset)), \
                     'page_offset'  : str(conv_hex(container.linux_page_offset)), \
                     'ztextaddr'    : str(conv_hex(container.linux_phys_offset)), \
                     'zreladdr'     : str(conv_hex(container.linux_zreladdr))})


        # Update ARCHID, CPUID and ATAGS ADDRESS
        cpuid = self.cpuid_list[config.cpu.upper()]
        archid = self.archid_list[config.platform.upper()]

        # Create system_macros header
        with open(self.system_macros_h_out, 'w+') as output:
            with open(self.system_macros_h_in, 'r') as input:
                output.write(input.read() % \
                    {'cpuid'        : cpuid, \
                     'archid'       : archid, \
                     'atags'        : str(conv_hex(container.linux_page_offset + 0x100))})

    def clean(self):
        os.system('rm -f ' + self.system_macros_h_out)
        os.system('rm -f ' + self.kconfig_out)

class LinuxBuilder:
    def __init__(self, pathdict, container, opts):
        self.LINUX_KERNELDIR = pathdict['LINUX_KERNELDIR']

        # Calculate linux kernel build directory
        self.LINUX_KERNEL_BUILDDIR = \
            source_to_builddir(LINUX_KERNELDIR, container.id)

        self.container = container
        self.kernel_binary_image = \
            join(os.path.relpath(self.LINUX_KERNEL_BUILDDIR, LINUX_KERNELDIR), \
                 'vmlinux')
        self.kernel_image = join(self.LINUX_KERNEL_BUILDDIR, 'linux.elf')
        self.kernel_updater = LinuxUpdateKernel(self.container)
        self.build_config_file = join(self.LINUX_KERNEL_BUILDDIR, '.config')
        self.platform_config_file = None

        # Default configuration file to use based on selected platform
        self.platform_config_files = {'PB926'   :   'versatile',
                                      'BEAGLE'  :   'omap3_beagle',
                                      'PBA9'    :   'vexpress_a9'}
        # This one is for EB, EB can have 1136/1176/11MPCore/A8/A9 coretiles
        self.cpu_config_file = {'CORTEXA8': 'eb-a8',
                                'CORTEXA9': 'eb-a9',
                                'ARM1136' : 'eb-1136',
                                'ARM11MPCORE': 'eb-11mpcore'}

    def print_verbose(self, text):
	    print "########################################################"
	    print "########################################################"
	    print "# " + text
	    print "########################################################"
	    print "########################################################"

    def defconfig_to_config(self, config):
        # First get the linux configuration file corresponding to chosen platform
        self.platform_defconfig = ''
        if config.platform.upper() == 'EB':
            self.platform_defconfig = self.cpu_config_file[config.cpu.upper()]
        else:
            self.platform_defconfig = self.platform_config_files[config.platform.upper()]

        if not self.platform_defconfig:
	    print 'Platform detected as: ' + config.platform
            print 'Could not find relevant linux config file please review configuration'
	    sys.exit(1)

	    # Create a config file from the corresponding defconfig
        os.system("make " + self.platform_defconfig + "_defconfig O=" + self.LINUX_KERNEL_BUILDDIR)

    def build_linux(self, config, opts):
        print '\nBuilding the linux kernel...'
        os.chdir(self.LINUX_KERNELDIR)
        if not os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            os.makedirs(self.LINUX_KERNEL_BUILDDIR)

        # Update linux configuration based on codezero config
        # TODO: This may be changed from run-always to run-on-modification.
        self.kernel_updater.update_kernel_params(config, self.container)

        # Is this the first time the kernel is to be configured?
        if not os.path.exists(self.build_config_file):
	    self.print_verbose("Config file does not exist. Creating from defconfig")
	    self.defconfig_to_config(config)

        # Batch mode runs without invoking configure stage
        if opts.batch:
            # Build the kernel directly
            os.system('make ARCH=arm CROSS_COMPILE=' + config.toolchain_kernel + \
                      ' O=' + self.LINUX_KERNEL_BUILDDIR + ' ' + self.build_config_file)
        else:
	        # Configure the kernel
    	    os.system('make ARCH=arm CROSS_COMPILE=' + config.toolchain_kernel + \
                      ' O=' + self.LINUX_KERNEL_BUILDDIR + ' menuconfig')
	    if not os.path.exists(self.build_config_file):
                self.print_verbose("Config file doesnt exist after building: " + self.build_config_file)
		sys.exit(1)

        # Build the kernel
        os.system('make V=1 ARCH=arm ' + '-j ' + opts.jobs + \
                  ' CROSS_COMPILE=' + config.toolchain_kernel + \
                  ' O=' + self.LINUX_KERNEL_BUILDDIR + ' Image')

        # Generate kernel_image, elf to be used by codezero
        linux_elf_gen_cmd = (config.toolchain_userspace + 'objcopy -R .note \
            -R .note.gnu.build-id -R .comment -S --change-addresses ' + \
            str(conv_hex(-self.container.linux_page_offset + self.container.linux_phys_offset)) + \
            ' ' + self.kernel_binary_image + ' ' + self.kernel_image)

        #print cmd
        os.system(linux_elf_gen_cmd)
        print 'Done...'

    def clean(self, config):
        print 'Cleaning linux kernel build...'
        self.kernel_updater.clean()
        os.system('rm -f ' + self.kernel_image)
        os.chdir(self.LINUX_KERNELDIR)
        os.system('make ARCH=arm CROSS_COMPILE=' + config.toolchain_kernel + \
                  ' O=' + self.LINUX_KERNEL_BUILDDIR + ' clean')
        print 'Done...'

if __name__ == '__main__':
    # This is only a default test case
    container = Container()
    container.id = 0
    linux_builder = LinuxBuilder(projpaths, container)

    if len(sys.argv) == 1:
        linux_builder.build_linux()
    elif 'clean' == sys.argv[1]:
        linux_builder.clean()
    else:
        print ' Usage: %s [clean]' % (sys.argv[0])

