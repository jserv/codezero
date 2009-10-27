# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- Virtualization microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, shelve
import configure
from configure import *
from os.path import *

config = configuration_retrieve()
arch = config.arch
subarch = config.subarch
platform = config.platform
gcc_cpu_flag = config.gcc_cpu_flag
all_syms = config.all

env = Environment(CC = config.kernel_toolchain + 'gcc',
		  # We don't use -nostdinc because sometimes we need standard headers,
		  # such as stdarg.h e.g. for variable args, as in printk().
		  CCFLAGS = ['-g', '-nostdlib', '-ffreestanding', '-std=gnu99', '-Wall', \
                     '-Werror', ('-mcpu=' + gcc_cpu_flag)],
		  LINKFLAGS = ['-nostdlib', '-T' + "include/l4/arch/arm/linker.lds"],
		  ASFLAGS = ['-D__ASSEMBLY__'],
		  PROGSUFFIX = '.elf',			# The suffix to use for final executable
		  ENV = {'PATH' : os.environ['PATH']},	# Inherit shell path
		  LIBS = 'gcc',				# libgcc.a - This is required for division routines.
		  CPPPATH = "#include",
		  CPPFLAGS = '-include l4/config.h -include l4/macros.h -include l4/types.h -D__KERNEL__')

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
Alias('kernel', kernel_elf)
Depends(kernel_elf, 'include/l4/config.h')

