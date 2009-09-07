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

includeDirectory = 'include'
containersDirectory = 'conts'
toolsDirectory = 'tools'
cml2ToolsDirectory = toolsDirectory + '/cml2-tools'
buildDirectory = 'build'

cml2CompileRulesFile = buildDirectory + '/cml2Rules.out'
cml2ConfigPropertiesFile = buildDirectory + '/cml2Config.out'
cml2ConfigHeaderFile = buildDirectory + '/cml2Config.h'

configureHelpEntry = 'configure the build. This must be done before any other action is possible.'

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
        print '#### Warning ####: configure is a target on the command line, all the other targets are being ignored as this is a configure step.'
    Command('configure',  ['#configs/arm.cml'], performCML2Configuration)
    Clean('configure', buildDirectory)

    baseEnvironment = Environment(tools=[],
                                  targetHelpEntries = {'configure': configureHelpEntry},
                                  )

else :

    if not os.path.exists(cml2ConfigPropertiesFile):
        if GetOption('help'):
            print '''
The project has to be configured even to get the help on the project.
Running 'scons configure' starts the configuration system.  Typing x
causes the configuration system to terminate and write all the
configuration data.  'scons -h' will then print the project help.
'''
        else:
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

    #  Read in the toolchains data.  This defines the variable toolchains which is a dictionary of
    #  dictionaries.

    execfile('toolchains.py')

    baseEnvironment = Environment(tools = ['gnulink', 'gcc', 'gas', 'ar'],
                                  ENV = {'PATH': os.environ['PATH']},
                                  configFiles = ('#' + cml2CompileRulesFile,  '#' + cml2ConfigPropertiesFile,  '#' + cml2ConfigHeaderFile),
                                  CCFLAGS =  ['-g', '-nostdlib', '-ffreestanding', '-std=gnu99', '-Wall', '-Werror'],
                                  ASFLAGS = ['-D__ASSEMBLY__'],
                                  LINKFLAGS = ['-nostdlib'],
                                  PROGSUFFIX = '.axf',
                                  toolchains = toolchains,
                                  includeDirectory = includeDirectory,
                                  containersDirectory = containersDirectory,
                                  toolsDirectory = toolsDirectory,
                                  cml2ToolsDirectory = cml2ToolsDirectory,
                                  buildDirectory = buildDirectory,
                                  targetHelpEntries = {'configure': configureHelpEntry},
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
    baseEnvironment['CC'] = baseEnvironment['toolchains'][configuration.env['ARCH']][configuration.env['CPU']]['mainCompiler']
    ##
    ##  Using the cpu option changes the behaviour of the test execution, it generates an illegal instruction exception :-(
    ##
    #baseEnvironment.Append(CCFLAGS = ['-mcpu=' + baseEnvironment['toolchains'][configuration.env['ARCH']][baseEnvironment['CPU']]['cpuOption']])

##########  Build the libraries ########################

    libs = {}
    crts = {}
    for variant in ['baremetal']:
        (libs[variant], crts[variant]) = SConscript('libs/c/SConscript', variant_dir = buildDirectory + '/lib/c/' + variant, duplicate = 0,
                                                    exports = {'environment': baseEnvironment, 'variant': variant})

    baseEnvironment['baremetal_libc'] = libs['baremetal']
    baseEnvironment['baremetal_crt0'] = crts['baremetal']

    libelf = SConscript('libs/elf/SConscript', variant_dir = buildDirectory + '/lib/elf', duplicate = 0, exports = {'environment': baseEnvironment})

    Alias('libs', crts.values() + libs.values() + [libelf])
    baseEnvironment['targetHelpEntries']['libs'] = 'build the support libraries.'

##########  Build the kernel ########################

    startAxf = SConscript('src/SConscript', variant_dir = buildDirectory + '/kernel', duplicate = 0, exports = {'environment': baseEnvironment})

    Alias('kernel', startAxf)
    baseEnvironment['targetHelpEntries']['kernel'] = 'build the kernel itself.'

##########  Handle all the container creation #######################

    containers = SConscript(containersDirectory + '/linux/SConscript', variant_dir = buildDirectory + '/' + containersDirectory, duplicate = 0, exports = {'environment': baseEnvironment, 'startAxf': startAxf})

    Alias('containers', containers)
    baseEnvironment['targetHelpEntries']['containers'] = 'build all the containers.'

##########  Do the packing / create loadable ########################

    loader = SConscript('loader/SConscript', variant_dir = buildDirectory + '/loader', duplicate = 0,
                        exports = {'environment': baseEnvironment, 'images': [startAxf] + containers, 'libsInOrder':  [libelf, libs['baremetal'], 'gcc', libs['baremetal']]})

    Alias('final', loader)
    baseEnvironment['targetHelpEntries']['final'] = 'build all components and the final loadable.'

    #  The test does not terminate and Ctrl-C and Ctrl-Z have no effect.  Run the job in the background so
    #  the initiating terminal retains control and allows the process to be killed from this terminal.  Add
    #  the sleep to force SCons to wait until the test has run before it decides all targets are built and
    #  return to the prompt.  Remind the user they have a running process in the background.

    Command('runTest', loader, "qemu-system-arm -kernel $SOURCE -nographic -m 128 -M versatilepb & sleep 10 ; echo '####\\n#### You will need to kill the qemu-system-arm process\\n#### that is running in the background.\\n####\\n'")

##########  Other rules. ########################

    Default(crts.values() + libs.values() + [libelf, startAxf, containers, loader])

    Clean('.', [buildDirectory])

##########  Be helpful ########################

if len(COMMAND_LINE_TARGETS) != 0:
    Help('\n')
    for item in COMMAND_LINE_TARGETS:
        if baseEnvironment['targetHelpEntries'].has_key(item):
            Help('  ' + item + ' -- ' + baseEnvironment['targetHelpEntries'][item] + '\n')
        elif FindFile(item, '.'):
            Help('  ' + item + ' is a file that exists or can be created.\n')
        else:
            Help('  ' + item + ' is not a possible target.\n')
else:
    Help('''
Possible targets are:
''')
    targetList = baseEnvironment['targetHelpEntries'].keys()
    targetList.sort()
    for item in targetList:
        Help('  ' + item + ' -- ' + baseEnvironment['targetHelpEntries'][item] + '\n')
    Help('''
The default target is 'final'.

A configure must have been executed before any other target is possible.
''')
