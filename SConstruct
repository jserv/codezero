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

arch = 'arm'
platform = 'pb926'

environment = Environment (
    CC = 'arm-none-linux-gnueabi-gcc',
    CCFLAGS = [ '-g' , '-nostdinc' , '-nostdlib' , '-ffreestanding' ] ,
    LINKFLAGS = [ '-nostdlib' ] ,
    ENV = { 'PATH' : os.environ['PATH'] } ,
    LIBS = 'gcc' ,
    ARCH = arch , 
    PLATFORM = platform )

Export ( 'environment' )

libs = { }
crts = { }
for variant in [ 'baremetal' , 'userspace' ] :
    ( libs[variant] , crts[variant] ) = SConscript ( 'libs/c/SConscript' , variant_dir = 'build/lib/c/' + variant , duplicate = 0 , exports = { 'variant' : variant } )

libelf = SConscript ( 'libs/elf/SConscript' , variant_dir = 'build/lib/elf' , duplicate = 0 , exports = { 'lib' : libs['baremetal'] } )
