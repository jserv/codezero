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

import os

includeDirectory = 'include'
toolsDirectory = 'tools'
cml2ToolsDirectory = toolsDirectory + '/cml2-tools'
buildDirectory = 'build'

cml2CompileRulesFile = buildDirectory + '/cml2Rules.out'
cml2ConfigPropertiesFile = buildDirectory + '/cml2Config.out'
cml2ConfigHeaderFile = buildDirectory + '/cml2Config.h'

#  The choice of which parts of the kernel to compile and include in the build depends on the configuration
#  which is managed using CML2.  CML2 uses a base configuration file (currently #configs/arm.cml) to drive
#  an interaction with the user which results in a trio of files that specify the user choice.
#
#  cml2RulesFile is the pickled version of the source configuration driver.
#
#  cml2Config.out is a properties type representation of the user selected configuration data.
#
#  cml2Config.h is a C include file representation of the user selected configuration data derived from
#  cml2Config.out, it is essential for the compilation of the C code of the kernel and the tasks.
#
#  Since the DAG for building the kernel relies on the information from the configuration split the build
#  into two distinct phases as Autotools and Waf do, configure then build.  Achieve this by partitioning the
#  SCons DAG building in two depending on the command line.

if 'configure' in COMMAND_LINE_TARGETS  :

    def performCML2Configuration ( target , source , env ) :
        if not os.path.isdir ( buildDirectory ) : os.mkdir ( buildDirectory )
        os.system ( cml2ToolsDirectory + '/cmlcompile.py -o ' + cml2CompileRulesFile + ' ' + source[0].path )
        os.system ( cml2ToolsDirectory +  '/cmlconfigure.py -c -o ' + cml2ConfigPropertiesFile + ' ' + cml2CompileRulesFile )
        os.system ( toolsDirectory +  '/cml2header.py -o ' + cml2ConfigHeaderFile + ' -i ' + cml2ConfigPropertiesFile )

    if len ( COMMAND_LINE_TARGETS ) != 1 :
        print '#### Warning####: configure is part of the command line, all the other targets are being ignored as this is a configure step.'
    Command ( 'configure' ,  [ '#configs/arm.cml' ] , performCML2Configuration )
    Clean ( 'configure' , buildDirectory )

else :

    if not os.path.exists ( cml2ConfigPropertiesFile ) :
        print "####\n#### Configuration has not been undertaken, please run 'scons configure'.\n####"
        Exit ( )

    def processCML2Config ( ) :
        configItems = { }
        with file ( cml2ConfigPropertiesFile ) as configFile :
            for line in configFile :
                item = line.split ( '=' )
                if len ( item ) == 2 :
                    configItems[item[0].strip ( )] = ( item[1].strip ( ) == 'y' )
        return configItems
                
    baseEnvironment = Environment ( tools = [ 'gnulink' , 'gcc' , 'gas' , 'ar' ] )

    kernelSConscriptPaths = [ 'generic' , 'api' , 'lib' ]

    configuration = Configure ( baseEnvironment , config_h =  buildDirectory + '/l4/config.h' )
    configData = processCML2Config ( )
    arch = None
    platform = None
    subarch = None
    for key , value in configData.items ( ) :
        if value :
            items = key.split ( '_' )
            if items[0] == 'ARCH' :
                arch = items[1].lower ( )
    for key , value in configData.items ( ) :
        if value :
            items = key.split ( '_' )
            if items[0] == arch.upper ( ) :
                if items[1] == 'PLATFORM' :
                    platform = items[2].lower ( )
                if items[1] == 'SUBARCH' :
                    subarch = items[2].lower ( )
            if items[0] == 'DRIVER' :
                kernelSConscriptPaths.append ( 'drivers/' + ( 'irq' if items[1] == 'IRQCTRL' else items[1].lower ( ) ) + '/' + items[2].lower ( ) )
    configuration.Define ( '__ARCH__' , arch )
    configuration.Define ( '__PLATFORM__' , platform )
    configuration.Define ( '__SUBARCH__' , subarch )
    kernelSConscriptPaths += [
        'arch/' + arch ,
        'glue/' + arch ,
        'platform/' + platform ,
        'arch/' + arch + '/' + subarch ]
    configuration.env['ARCH'] = arch
    configuration.env['PLATFORM'] = platform
    configuration.env['SUBARCH'] = subarch
    baseEnvironment = configuration.Finish ( )

    libraryEnvironment = baseEnvironment.Clone (
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

    libelf = SConscript ( 'libs/elf/SConscript' , variant_dir = buildDirectory + '/lib/elf' , duplicate = 0 , exports = { 'environment' : libraryEnvironment } )

    kernelEnvironment = baseEnvironment.Clone (
        CC = 'arm-none-eabi-gcc' ,
        # We don't use -nostdinc because sometimes we need standard headers, such as stdarg.h e.g. for variable
        # args, as in printk().
        CCFLAGS = [ '-g' , '-mcpu=arm926ej-s' , '-nostdlib' , '-ffreestanding' , '-std=gnu99' , '-Wall' , '-Werror' ] ,
        LINKFLAGS = [ '-nostdlib' , '-T' +  includeDirectory + '/l4/arch/' + arch + '/mylink.lds' ] ,
        ASFLAGS = [ '-D__ASSEMBLY__' ] ,
        PROGSUFFIX = '.axf' ,
        ENV = { 'PATH' : os.environ['PATH'] } ,
        LIBS = 'gcc' ,  # libgcc.a is required for the division routines.
        CPPPATH = [ '#' + buildDirectory , '#' + buildDirectory + '/l4' , '#' + includeDirectory , '#' + includeDirectory + '/l4' ] ,
        CPPFLAGS = [ '-include' , 'config.h' , '-include' , 'cml2Config.h' , '-include' , 'macros.h' , '-include' , 'types.h' , '-D__KERNEL__' ] )
    
    kernelComponents = [ ]
    for scriptPath in [ 'src/' + path for path in kernelSConscriptPaths ] :
        kernelComponents.append ( SConscript ( scriptPath + '/SConscript' , variant_dir = buildDirectory + '/' + scriptPath , duplicate = 0 , exports = { 'environment' : kernelEnvironment } ) )

    startAxf = kernelEnvironment.Program ( buildDirectory + '/start.axf' ,  kernelComponents )

    tasksEnvironment = baseEnvironment.Clone (
        CC = 'arm-none-linux-gnueabi-gcc' ,
        CCFLAGS = [ '-g' , '-nostdlib' , '-Wall' , '-Werror' , '-ffreestanding' , '-std=gnu99' ] ,
        LINKFLAGS = [ '-nostdlib' ] ,
        ENV = { 'PATH' : os.environ['PATH'] } ,
        LIBS = 'gcc' ,
        CPPPATH = [ '#' + includeDirectory , '#' + buildDirectory + '/l4' , includeDirectory ] )

    tasks = [ ]
    for task in [ item for item in os.listdir ( 'tasks' ) if os.path.isdir ( 'tasks/' + item ) and os.path.exists ( 'tasks/' + item + '/SConscript' )] :
        tasks.append ( SConscript ( 'tasks/' + task + '/SConscript' , variant_dir = buildDirectory + '/tasks/' + task , duplicate = 0 , exports = { 'environment' : tasksEnvironment } ) )

    Default ( crts.values ( ) + libs.values ( ) + [ libelf , startAxf ] + tasks )
    
    Clean ( '.' , [ buildDirectory ] )
