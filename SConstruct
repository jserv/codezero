# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- Virtualization microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
#  This Script is mainly concerned with(in the same order):
#  1. Generating linker script for kernel.
#  2. Generating cinfo.c file based on generated containers.
#  3. Calling scons recursively over various kernel subdirectories.
#
import os
from os.path import *
from scripts.config.config_invoke import *

config          =   configuration_retrieve()
arch            =   config.arch
subarch         =   config.subarch
platform        =   config.platform
gcc_arch_flag   =   config.gcc_arch_flag
builddir        =   join(BUILDDIR, 'kernel')

# Generate kernel linker script at runtime using template file
def generate_kernel_linker_script(target, source, env):
    linker_in = source[0]
    linker_out = target[0]

    cmd = config.toolchain_kernel + "cpp -D__CPP__ " + \
          "-I%s -imacros l4/macros.h -imacros %s -imacros %s -C -P %s -o %s" % \
                ('include', 'l4/platform/'+ platform + '/offsets.h', \
                 'l4/glue/' + arch + '/memlayout.h', linker_in, linker_out)
    os.system(cmd)
    return None

'''
# Generate kernel linker file with physical addresses
# (to be used for debug purpose only)
def generate_kernel_phys_linker_script(target, source, env):
    phys_linker_in = source[0]
    phys_linker_out = target[0]

    cmd = config.toolchain_kernel + "cpp -D__CPP__ -Dkernel_offset=0 " + \
          "-I%s -imacros l4/macros.h -imacros %s -imacros %s -C -P %s -o %s" % \
                ('include', 'l4/platform/'+ platform + '/offsets.h', \
                 'l4/glue/' + arch + '/memlayout.h', phys_linker_in, phys_linker_out)
    os.system(cmd)
    return None
'''

# Kernel build environment
env = Environment(CC = config.toolchain_kernel + 'gcc',
	          AR = config.toolchain_kernel + 'ar',
	          RANLIB = config.toolchain_kernel + 'ranlib',
	          # We don't use -nostdinc because sometimes we need standard headers,
	          # such as stdarg.h e.g. for variable args, as in printk().
	          CCFLAGS = ['-g', '-nostdlib', '-ffreestanding', '-std=gnu99',
                             '-Wall', '-Werror', '-march=' + gcc_arch_flag],
	          LINKFLAGS = ['-nostdlib',
                               '-T' + join(builddir, 'include/l4/arch/arm/linker.lds')],
	          ASFLAGS = ['-D__ASSEMBLY__', '-march=' + gcc_arch_flag],
	          # The suffix to use for final executable
                  PROGSUFFIX = '.elf',
                  # Inherit shell path
	          ENV = {'PATH' : os.environ['PATH']},
	          # libgcc.a - This is required for division routines.
                  LIBS = 'gcc',
	          CPPPATH = KERNEL_HEADERS,
	          CPPFLAGS = '-include l4/config.h -include l4/macros.h \
                              -include l4/types.h -D__KERNEL__')

objects = []
objects += SConscript('src/generic/SConscript',
                      exports = { 'env' : env }, duplicate = 0,
                      variant_dir = join(builddir, 'generic'))

objects += SConscript(join(join('src/glue', arch), 'SConscript'),
                      exports = { 'env' : env }, duplicate = 0,
                      variant_dir = join(builddir, join('glue',arch)))

objects += SConscript(join(join('src/arch', arch), 'SConscript'),
                      exports = { 'env' : env }, duplicate = 0,
                      variant_dir = join(builddir, join('arch', arch)))

objects += SConscript(join(join('src/arch', arch), join(subarch, 'SConscript')),
                      exports = { 'env' : env }, duplicate = 0,
                      variant_dir = join(builddir, join(join('arch',arch), subarch)))

objects += SConscript('src/lib/SConscript',
                      exports = { 'env' : env }, duplicate = 0,
                      variant_dir = join(builddir, 'lib'))

objects += SConscript('src/api/SConscript',
                      exports = { 'env' : env }, duplicate = 0,
                      variant_dir = join(builddir, 'api'))

objects += SConscript('src/drivers/SConscript',
                      exports = { 'env' : env, 'bdir' : 'driver/'}, duplicate = 0,
                      variant_dir = join(builddir, 'driver'))

objects += SConscript(join(join('src/platform', platform), 'SConscript'),
                      exports = { 'env' : env }, duplicate = 0,
                      variant_dir = join(builddir, join('platform', platform)))


# Add builders for generating kernel linker scripts
kernel_linker = Builder(action = generate_kernel_linker_script)
env.Append(BUILDERS = {'KERNEL_LINKER' : kernel_linker})
env.KERNEL_LINKER(join(builddir, 'include/l4/arch/arm/linker.lds'),
                  [join(PROJROOT, 'include/l4/arch/arm/linker.lds.in'), CONFIG_H])

'''
kernel_phys_linker = Builder(action = generate_kernel_phys_linker_script)
env_phys = env.Clone()
env_phys.Replace(LINKFLAGS = ['-nostdlib', '-T' + join(builddir, 'include/physlink.lds')])
env_phys.Append(BUILDERS = {'KERNEL_LINKER' : kernel_phys_linker})
env_phys.KERNEL_LINKER(join(builddir, 'include/physlink.lds'),
                       join(PROJROOT, 'include/l4/arch/arm/linker.lds.in'), CONFIG_H)

kernel_phys_elf = env_phys.Program(join(BUILDDIR, 'kernel_phys.elf'), objects)
Depends(kernel_phys_elf, objects)
'''

kernel_elf = env.Program(KERNEL_ELF, objects)
Depends(objects, CONFIG_H)
