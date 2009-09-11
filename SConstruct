# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, shelve
import configure
from configure import *

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

config_shelve = shelve.open(CONFIG_SHELVE)
#symbols = config_shelve["config_symbols"]
arch = config_shelve["arch"]
subarch = config_shelve["subarch"]
platform = config_shelve["platform"]
all_syms = config_shelve["all_symbols"]

print all_syms
'''
sources = \
        Glob('src/api/*.[cS]') + \
        Glob('src/generic/*.[cS]') + \
        Glob('src/lib/*.[cS]') + \
        Glob('src/arch/' + arch + '/*.[cS]') + \
        Glob('src/arch/' + arch + '/' + subarch +'/*.[cS]') + \
        Glob('src/glue/' + arch + '/*.[cS]') + \
        Glob('src/platform/' + platform + '/*.[cS]')
print sources
'''

driver_objs = SConscript('src/drivers/SConscript', duplicate = 0, \
                         variant_dir = BUILD
                         exports = {'symbols' : all_syms, 'env' : env})
generic_objs = SConscript('src/generic/SConscript', duplicate = 0, \
                         variant_dir = BUILDDIR + "/src/generic", \
                         exports = {'symbols' : all_syms, 'env' : env})
arch_objs = SConscript('src/arch/' + arch + '/SConscript', duplicate = 0, \
                         variant_dir = BUILDDIR + "/src/arch/" + arch, \
                         exports = {'symbols' : all_syms, 'env' : env})
plat_objs = SConscript('src/platform/' + platform + '/SConscript', duplicate = 0, \
                         variant_dir = BUILDDIR + "/src/platform/" + platform, \
                         exports = {'symbols' : all_syms, 'env' : env})
subarch_objs = SConscript('src/arch/' + arch + '/' + subarch + '/SConscript',  \
                         variant_dir = BUILDDIR + '/src/arch/' + arch + '/' + subarch, \
                         exports = {'symbols' : all_syms, 'env' : env})
glue_objs = SConscript('src/glue/' + arch + '/SConscript', duplicate = 0, \
                         variant_dir = BUILDDIR + '/src/glue/' + arch, \
                         exports = {'symbols' : all_syms, 'env' : env})
lib_objs = SConscript('src/lib/SConscript', duplicate = 0, \
                         variant_dir = BUILDDIR + '/src/lib', \
                         exports = {'symbols' : all_syms, 'env' : env})
api_objs = SConscript('src/api/SConscript', duplicate = 0, \
                         variant_dir = BUILDDIR + '/src/api', \
                         exports = {'symbols' : all_syms, 'env' : env})

objects = driver_objs + generic_objs + arch_objs + plat_objs + subarch_objs + \
          glue_objs + lib_objs + api_objs

kernel_elf = env.Program('kernel.elf', objects)


