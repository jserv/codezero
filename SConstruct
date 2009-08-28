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

#  To support Python 2.5 we need the following, which seems to do no harm in Python 2.6.  Only if Python 2.6
#  is the floor version supported can this be dispensed with.

from __future__ import with_statement

import os

posixServicesDirectory = "containers/posix/"
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

    def performCML2Configuration(target, source, env):
        if not os.path.isdir(buildDirectory) : os.mkdir(buildDirectory)
        os.system(cml2ToolsDirectory + '/cmlcompile.py -o ' + cml2CompileRulesFile + ' ' + source[0].path)
        os.system(cml2ToolsDirectory +  '/cmlconfigure.py -c -o ' + cml2ConfigPropertiesFile + ' ' + cml2CompileRulesFile)
        os.system(toolsDirectory +  '/cml2header.py -o ' + cml2ConfigHeaderFile + ' -i ' + cml2ConfigPropertiesFile)

    if len(COMMAND_LINE_TARGETS) != 1:
        print '#### Warning####: configure is part of the command line, all the other targets are being ignored as this is a configure step.'
    Command('configure',  ['#configs/arm.cml'], performCML2Configuration)
    Clean('configure', buildDirectory)

else :

    if not os.path.exists(cml2ConfigPropertiesFile):
        print "####\n#### Configuration has not been undertaken, please run 'scons configure'.\n####"
        Exit()

##########  Create the base environment and process the configuration ########################

    def processCML2Config():
        configItems = {}
        with file(cml2ConfigPropertiesFile) as configFile:
            for line in configFile:
                item = line.split('=')
                if len(item) == 2:
                    configItems[item[0].strip()] = (item[1].strip() == 'y')
        return configItems

    baseEnvironment = Environment(tools = ['gnulink', 'gcc', 'gas', 'ar'],
                                  ENV = {'PATH': os.environ['PATH']},
                                  configFiles = ('#' + cml2CompileRulesFile,  '#' + cml2ConfigPropertiesFile,  '#' + cml2ConfigHeaderFile),
                                  CCFLAGS =  ['-g', '-nostdlib', '-ffreestanding', '-std=gnu99', '-Wall', '-Werror'],
                                  ASFLAGS = ['-D__ASSEMBLY__'],
                                  LINKFLAGS = ['-nostdlib'],
                                  PROGSUFFIX = '.axf',
                                  toolChains = {
                                      'arm926': {
                                          'mainCompiler': 'arm-none-linux-gnueabi-gcc',
                                          'kernelCompiler': 'arm-none-eabi-gcc',
                                          'cpuOption': 'arm926ej-s'}
                                      },
                                  )

    #  It is assumed that the C code is assuming that the configuration file will be found at l4/config.h so create it there.
    #
    #  Kernel code include config.h in a different way to all the other bits of code.
    #
    #  TODO:  Decide if this is an issue or not.

    configHPath = buildDirectory + '/l4/config.h'
    configuration = Configure(baseEnvironment, config_h =  configHPath)
    configData = processCML2Config()
    for key, value in configData.items():
        if value:
            items = key.split('_')
            if items[0] == 'ARCH':
                configuration.env['ARCH'] = items[1].lower()
    for key, value in configData.items():
        if value:
            items = key.split('_')
            if items[0] == 'ARCH': continue
            if items[0] == configuration.env['ARCH'].upper():
                configuration.env[items[1]] = items[2].lower()
            else:
                path = items[1].lower() + '/' + items[2].lower()
                try:
                    configuration.env[items[0]]
                except KeyError:
                    configuration.env[items[0]] = []
                configuration.env[items[0]].append(path)
    configuration.Define('__ARCH__', configuration.env['ARCH'])
    configuration.Define('__PLATFORM__', configuration.env['PLATFORM'])
    configuration.Define('__SUBARCH__', configuration.env['SUBARCH'])
    baseEnvironment = configuration.Finish()
    baseEnvironment.Append(configFiles = ('#' + configHPath,))
    baseEnvironment['CC'] = baseEnvironment['toolChains'][configuration.env['CPU']]['mainCompiler']
    #baseEnvironment.Append(CCFLAGS = ['-mcpu=' + baseEnvironment['toolChains'][baseEnvironment['CPU']]['cpuOption']])

##########  Build the libraries ########################

    libraryEnvironment = baseEnvironment.Clone()
    libraryEnvironment.Append(CCFLAGS = '-nostdinc')
    libraryEnvironment.Append(LIBS = ['gcc'])

    libs = {}
    crts = {}
    for variant in ['baremetal']:
        (libs[variant], crts[variant]) = SConscript('libs/c/SConscript', variant_dir = buildDirectory + '/lib/c/' + variant, duplicate = 0, exports = {'environment': libraryEnvironment, 'variant': variant})

    baseEnvironment['baremetal_libc'] = libs['baremetal']
    baseEnvironment['baremetal_crt0'] = crts['baremetal']

    libelf = SConscript('libs/elf/SConscript', variant_dir = buildDirectory + '/lib/elf', duplicate = 0, exports = {'environment': libraryEnvironment})

    Alias('libs', crts.values() + libs.values() + [libelf])

##########  Build the kernel ########################

    kernelEnvironment = baseEnvironment.Clone()
    kernelEnvironment['CC'] = baseEnvironment['toolChains'][baseEnvironment['CPU']]['kernelCompiler']
    kernelEnvironment.Append(LINKFLAGS = ['-T' +  includeDirectory + '/l4/arch/' + baseEnvironment['ARCH'] + '/linker.lds'])
    kernelEnvironment.Append(LIBS = ['gcc'])
    kernelEnvironment.Append(CPPPATH = ['#' + buildDirectory, '#' + buildDirectory + '/l4', '#' + includeDirectory, '#' + includeDirectory + '/l4'])
    ####
    ####  TODO:  Why are these files forcibly included, why not just leave it up to the C code to include things?
    ####
    kernelEnvironment.Append(CPPFLAGS = ['-include', 'config.h', '-include', 'cml2Config.h', '-include', 'macros.h', '-include', 'types.h', '-D__KERNEL__'])

    startAxf = SConscript('src/SConscript' , variant_dir = buildDirectory + '/kernel' , duplicate = 0, exports = {'environment': kernelEnvironment})

    Alias('kernel', startAxf)

##########  Build the task libraries ########################

    taskSupportLibraryEnvironment = baseEnvironment.Clone()
    taskSupportLibraryEnvironment.Append(LIBS = ['gcc'])
    taskSupportLibraryEnvironment.Append(CPPPATH = ['#' + buildDirectory, '#' + buildDirectory + '/l4', '#' + includeDirectory])

    taskLibraryNames = [f.name for f in Glob(posixServicesDirectory + 'lib*')]

    taskLibraries = []
    for library in taskLibraryNames:
        taskLibraries.append(SConscript(posixServicesDirectory + library + '/SConscript', variant_dir = buildDirectory + '/' + posixServicesDirectory + library, duplicate = 0, exports = {'environment': taskSupportLibraryEnvironment, 'posixServicesDirectory': posixServicesDirectory}))

    Alias ('tasklibs', taskLibraries)

##########  Build the tasks ########################

    def buildTask(programName, sources, environment, previousImage, extraCppPath=None):
        e = environment.Clone()
        e.Append(LINKFLAGS=['-T' + posixServicesDirectory + programName + '/include/linker.lds'])
        e.Append(LIBPATH=['#build/' + posixServicesDirectory + programName])
        if extraCppPath: e.Append(CPPPATH=extraCppPath)
        objects = e.StaticObject(sources)
        Depends(objects, e['configFiles'])
        program = e.Program(programName, objects)
        environment['physicalBaseLinkerScript'] = Command('include/physical_base.lds', previousImage, 'tools/pyelf/readelf.py --first-free-page ' + previousImage[0].path + ' >> $TARGET')
        Depends(program, [environment['physicalBaseLinkerScript']])
        return program

    tasksEnvironment = baseEnvironment.Clone()
    tasksEnvironment.Append(LIBS =  taskLibraries + ['gcc'] + taskLibraries)
    tasksEnvironment.Append(CPPDEFINES = ['__USERSPACE__'])
    tasksEnvironment.Append(CPPPATH = ['#' + buildDirectory, '#' + buildDirectory + '/l4', '#' + includeDirectory, 'include', \
	'#' + posixServicesDirectory + 'libl4/include', '#' + posixServicesDirectory + 'libc/include', \
	'#' + posixServicesDirectory + 'libmem', '#' + posixServicesDirectory + 'libposix/include'])
    tasksEnvironment.Append(buildTask = buildTask)

####
####  TODO: Why does the linker require crt0.o to be in the current directory and named as such.  Is it
####  because of the text in the linker script?
####
    
    #### taskNameList = [ f.name for f in Glob(posixServicesDirectory + '*') if f.name not in taskLibraryNames + ['bootdesc'] ]
    #### imageOrderData = [(taskName, []) for taskName in taskNameList]
    execfile(posixServicesDirectory + 'taskOrder.py')
    imageOrderData = [(taskName, []) for taskName in taskOrder]
    imageOrderData[0][1].append(startAxf)
    tasks = []
    for i in range(len(imageOrderData)):
        taskName = imageOrderData[i][0]
        dependency = imageOrderData[i][1]
        program = SConscript(posixServicesDirectory + taskName + '/SConscript', variant_dir = buildDirectory + '/' + posixServicesDirectory + taskName, duplicate = 0, exports = {'environment': tasksEnvironment, 'previousImage': dependency[0], 'posixServicesDirectory':posixServicesDirectory})
        tasks.append(program)
        if i < len(imageOrderData) - 1:
            imageOrderData[i+1][1].append(program)

    Alias ('tasks', tasks)

##########  Create the boot description ########################

    taskName = 'bootdesc'

    bootdescEnvironment = baseEnvironment.Clone()
    bootdescEnvironment.Append(LINKFLAGS = ['-T' + posixServicesDirectory + taskName + '/linker.lds'])
    bootdescEnvironment.Append(LIBS = ['gcc'])
    bootdescEnvironment.Append(CPPPATH = ['#' + includeDirectory])

    bootdesc = SConscript(posixServicesDirectory + taskName + '/SConscript', variant_dir = buildDirectory + '/' + posixServicesDirectory + taskName, duplicate = 0, exports = {'environment': bootdescEnvironment, 'images': [startAxf] + tasks})

    Alias('bootdesc', bootdesc)

##########  Do the packing / create loadable ########################

    loaderEnvironment = baseEnvironment.Clone()
    loaderEnvironment.Append(LINKFLAGS = ['-T' + buildDirectory + '/loader/linker.lds'])
    loaderEnvironment.Append(LIBS = [libelf, libs['baremetal'], 'gcc', libs['baremetal']])
    loaderEnvironment.Append(CPPPATH = ['#libs/elf/include', '#' + buildDirectory + '/loader'])

    ####  TODO: Fix the tasks data structure so as to avoid all the assumptions.

    loader = SConscript('loader/SConscript', variant_dir = buildDirectory + '/loader', duplicate = 0, exports = {'environment': loaderEnvironment, 'images':[startAxf, bootdesc] + tasks, 'posixServicesDirectory': posixServicesDirectory})

    Alias('final', loader)

    #  The test does not terminate and Ctrl-C and Ctrl-Z have no effect.  Run the job in the background so
    #  the initiating terminal retains control and allows the process to be killed from this terminal.  Add
    #  the sleep to force SCons to wait until the test has run before it decides all targets are built and
    #  return to the prompt.  Remind the user they have a running process in the background.

    Command('runTest', loader, "qemu-system-arm -kernel $SOURCE -nographic -m 128 -M versatilepb & sleep 10 ; echo '####\\n#### You will need to kill the qemu-system-arm process\\n#### that is running in the background.\\n####\\n'")

##########  Other rules. ########################

    Default(crts.values() + libs.values() + [libelf, startAxf] + tasks + bootdesc + loader)

    Clean('.', [buildDirectory])

##########  Be helpful ########################

Help('''
Explicit targets are:

  configure -- configure the build area ready for a build.

  libs -- build the support library.
  kernel -- build the kernel.
  tasklibs -- build all the support libraries for the tasks.
  tasks -- build all the tasks.
  bootdesc -- build the tasks and the boot descriptor.
  final -- build the loadable.

The default target is to compile everything and to do a final link.

Compilation can only be undertaken after a configuration.
''')
