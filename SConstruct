# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, shelve
import configure
from configure import *
from os.path import *

env = Environment(CC = 'arm-none-eabi-gcc',
		  # We don't use -nostdinc because sometimes we need standard headers,
		  # such as stdarg.h e.g. for variable args, as in printk().
		  CCFLAGS = ['-g', '-mcpu=arm926ej-s', '-nostdlib', '-ffreestanding', \
			     '-std=gnu99', '-Wall', '-Werror'],
		  LINKFLAGS = ['-nostdlib', '-T' + "include/l4/arch/arm/linker.lds"],
		  ASFLAGS = ['-D__ASSEMBLY__'],
		  PROGSUFFIX = '.elf',			# The suffix to use for final executable
		  ENV = {'PATH' : os.environ['PATH']},	# Inherit shell path
		  LIBS = 'gcc',				# libgcc.a - This is required for division routines.
		  CPPPATH = "#include",
		  CPPFLAGS = '-include l4/config.h -include l4/macros.h -include l4/types.h -D__KERNEL__')


config = configuration_retrieve()
arch = config.arch
subarch = config.subarch
platform = config.platform
all_syms = config.all

objects = []
objects += SConscript('src/drivers/SConscript', exports = {'symbols' : all_syms, 'env' : env})
objects += SConscript('src/generic/SConscript',exports = {'symbols' : all_syms, 'env' : env})
objects += SConscript('src/arch/' + arch + '/SConscript', exports = {'symbols' : all_syms, 'env' : env})
objects += SConscript('src/platform/' + platform + '/SConscript', exports = {'symbols' : all_syms, 'env' : env})
objects += SConscript('src/arch/' + arch + '/' + subarch + '/SConscript', exports = {'symbols' : all_syms, 'env' : env})
objects += SConscript('src/glue/' + arch + '/SConscript', exports = {'symbols' : all_syms, 'env' : env})
objects += SConscript('src/lib/SConscript', exports = {'symbols' : all_syms, 'env' : env})
objects += SConscript('src/api/SConscript', exports = {'symbols' : all_syms, 'env' : env})

kernel_elf = env.Program(BUILDDIR + '/kernel.elf', objects)

#libl4 = SConscript('conts/libl4/SConscript', \
#                   exports = { 'arch' : arch }, duplicate = 0, \
#                   variant_dir = join(BUILDDIR, os.path.relpath('conts/libl4', PROJROOT)))
