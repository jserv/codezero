# -*- mode: python; coding: utf-8; -*-

#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
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

#  This dictionary, specifies all the toolchain properties.  There must be an entry in this dictionary for
#  each and every processor type specified in configs/arm.cml.

toolchains = {
    'arm' : {
        'arm1136': {
            'mainCompiler': 'arm-none-linux-gnueabi-gcc',
            'kernelCompiler': 'arm-none-eabi-gcc',
            'cpuOption': 'arm1136j-s', # What about arm1136jf-s ???
            },
        'arm11mpcore': {
            'mainCompiler': 'arm-none-linux-gnueabi-gcc',
            'kernelCompiler': 'arm-none-eabi-gcc',
            'cpuOption': 'mpcore'
            },
        'arm926': {
            'mainCompiler': 'arm-none-linux-gnueabi-gcc',
            'kernelCompiler': 'arm-none-eabi-gcc',
            'cpuOption': 'arm926ej-s'
            },
        'cortexa8': {
            'mainCompiler': 'arm-none-linux-gnueabi-gcc',
            'kernelCompiler': 'arm-none-eabi-gcc',
            'cpuOption': 'cortex-a8'
            },
        },
    }
