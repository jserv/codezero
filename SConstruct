# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, shelve
from configure import configure_kernel

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

config_shelve = shelve.open("build/configdata/configuration")
configuration = config_shelve["cml2_config"]
cf = configuration

sources = \
        Glob('src/api/*.[cS]') + \
        Glob('src/generic/*.[cS]') + \
        Glob('src/lib/*.[cS]') + \
        Glob('src/arch/' + cf['ARCH'] + '/*.[cS]') + \
        Glob('src/arch/' + cf['ARCH'] + '/' + cf['SUBARCH'] +'/*.[cS]') + \
        Glob('src/glue/' + cf['ARCH'] + '/*.[cS]') + \
        Glob('src/platform/' + cf['PLATFORM'] + '/*.[cS]')

for item in cf['DRIVER'] :
    path = 'src/drivers/' + item
    if not os.path.isdir(path):
        feature , device = item.split ( '/' )
        raise ValueError, 'Driver ' + device + ' for ' + feature + ' not available.'
    sources += Glob(path + '/*.[cS]')

objects = env.Object(sources)
startAxf = env.Program('start.axf', objects)

