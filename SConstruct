# -*- mode: python; coding: utf-8; -*-

#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009 B Labs Ltd
#
#  This program is free software: you can redistribute it and/or modify it under the terms of the GNU
#  General Public License as published by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
#  License for more details.
#
#  You should have received a copy of the GNU General Public License along with this program.  If not, see
#  <http://www.gnu.org/licenses/>.
#
#  Author: Russel Winder

import os

includeDirectory = 'include'
toolsDirectory = 'tools'
cml2ToolsDirectory = toolsDirectory + '/cml2-tools'

buildDirectory = 'build'

environment = Environment ( )

configuration = Configure ( environment , config_h = 'config.h' )
environment = configuration.Finish ( )

arch = 'arm'
platform = 'pb926'

linkerScript = includeDirectory + '/l4/arch/' + arch + '/mylinks.lds'

libraryEnvironment = environment.Clone (
    CC = 'arm-none-linux-gnueabi-gcc',
    CCFLAGS = [ '-g' , '-nostdinc' , '-nostdlib' , '-ffreestanding' ] ,
    LINKFLAGS = [ '-nostdlib' ] ,
    ENV = { 'PATH' : os.environ['PATH'] } ,
    LIBS = 'gcc' ,
    ARCH = arch , 
    PLATFORM = platform )

libs = { }
crts = { }
for variant in [ 'baremetal' , 'userspace' ] :
    ( libs[variant] , crts[variant] ) = SConscript ( 'libs/c/SConscript' , variant_dir = buildDirectory + '/lib/c/' + variant , duplicate = 0 , exports = { 'environment' : libraryEnvironment , 'variant' : variant } )

libelf = SConscript ( 'libs/elf/SConscript' , variant_dir = buildDirectory + '/lib/elf' , duplicate = 0 , exports = { 'environment' : libraryEnvironment , 'lib' : libs['baremetal'] } )

cmlCompiledRules = Command ( '#' + buildDirectory + '/rules.out' , '#configs/' + arch + '.cml' , cml2ToolsDirectory + '/cmlcompile.py -o $TARGET $SOURCE' )
cmlConfiguredRules = Command ( '#' + buildDirectory + '/config.out' , cmlCompiledRules , cml2ToolsDirectory + '/cmlconfigure.py -c -o $TARGET $SOURCE' )
cmlConfigHeader = Command ( '#' + buildDirectory + '/config.h' , cmlConfiguredRules , toolsDirectory + '/cml2header.py -o $TARGET -i $SOURCE' )

Alias ( 'configure' , cmlConfigHeader )
if 'configure' in COMMAND_LINE_TARGETS:
    AlwaysBuild ( cmlConfiguredRules )

kernelEnvironment = environment.Clone (
    CC = 'arm-none-eabi-gcc' ,
    # We don't use -nostdinc because sometimes we need standard headers, such as stdarg.h e.g. for variable
    # args, as in printk().
    CCFLAGS = [ '-g' , '-mcpu=arm926ej-s' , '-nostdlib' , '-ffreestanding' , '-std=gnu99' , '-Wall' , '-Werror' ] ,
    LINKFLAGS = [ '-nostdlib' , '-T' + linkerScript ] ,
    ASFLAGS = [ '-D__ASSEMBLY__' ] ,
    PROGSUFFIX = '.axf' ,
    ENV = { 'PATH' : os.environ['PATH'] } ,
    LIBS = 'gcc' ,  # libgcc.a is required for the division routines.
    CPPPATH = includeDirectory ,
    CPPFLAGS = [ '-include l4/config.h' , '-include l4/macros.h' , '-include l4/types.h' , '-D__KERNEL__' ] )

Clean ( '.' , [ buildDirectory ] )
