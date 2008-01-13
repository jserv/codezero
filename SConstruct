#
# Almost enough mechanism to build a toy OS
#
# Copyright (C) 2007 Bahadir Balban
#
import os
import sys
import shutil
from string import split
from os.path import join
from os import sys
import glob

# The root directory of the repository where this file resides:
project_root = os.getcwd()
source_root = os.path.join(project_root, 'src')
headers_root = os.path.join(project_root, 'include')
project_tools_root = os.path.join(project_root, 'tools')
cml2_tools_root = os.path.join(project_tools_root, 'cml2-tools')
config_h = os.path.join(headers_root, "l4/config.h")

# Make sure python modules can be imported from tools and cml-tools
sys.path.append(project_tools_root)
sys.path.append(cml2_tools_root)

import cml2header
import cmlconfigure

# The linker script to link the final executable
linker_script = join(headers_root, 'l4/arch/arm/mylink.lds')

# Environment to build sources
#env = None

###########################################
#					  #
# 	ARM kernel build environment	  #
#					  #
###########################################

kernel_env = Environment(CC = 'arm-none-eabi-gcc',
		  # We don't use -nostdinc because sometimes we need standard headers,
		  # such as stdarg.h e.g. for variable args, as in printk().
		  CCFLAGS = ['-g', '-mcpu=arm926ej-s', '-nostdlib', '-ffreestanding', \
			     '-std=gnu99', '-Wall', '-Werror'],
		  LINKFLAGS = ['-nostdlib', '-T' + linker_script],
		  ASFLAGS = ['-D__ASSEMBLY__'],
		  PROGSUFFIX = '.axf',			# The suffix to use for final executable
		  ENV = {'PATH' : os.environ['PATH']},	# Inherit shell path
		  LIBS = 'gcc',				# libgcc.a - This is required for division routines.
		  CPPPATH = headers_root,
		  CPPFLAGS = '-include l4/config.h -include l4/macros.h -include l4/types.h -D__KERNEL__')

#########################################
#					#
#	TEST BUILDING ENVIRONMENT	#
#    					#
#########################################

# The purpose of test build is somewhat similar to regression/unit
# testing. There are individual tests for each interesting
# function/module (e.g. memory allocator layers). For each individual
# test only the relevant sources' SConscript files are picked up.
# So we don't build a whole kernel, but many little individual tests.
tests_env = Environment(CC = 'gcc -m32',
		  CCFLAGS = ['-g', '-std=gnu99', '-Wall', '-Werror'],
		  ENV = {'PATH' : os.environ['PATH']},
		  LIBS = 'gcc',
		  CPPPATH = '#include',
		  CPPFLAGS = '-include l4/config.h -include l4/macros.h -include l4/types.h -D__KERNEL__')

# Dictionary mappings for configuration symbols to directories of
# sources that relate to those symbols.
arch_to_dir =    { 'ARCH_ARM' : 'arm', 			\
		   'ARCH_TEST' : 'tests' }
plat_to_dir =    { 'ARM_PLATFORM_PB926' : 'pb926', 	\
		   'TEST_PLATFORM' : 'tests' }
subarch_to_dir = { 'ARM_SUBARCH_V5' : 'v5', 		\
		   'TEST_SUBARCH' : 'tests' }
driver_to_dir =  { 'DRIVER_UART_PL011' : 'uart/pl011',	\
		   'DRIVER_TIMER_SP804' : 'timer/sp804', \
		   'DRIVER_IRQCTRL_PL190' : 'irq/pl190' }

arch_to_env =	 { 'arm' : kernel_env,		\
		   'tests' : tests_env }

def kprocess_config_symbols(config_symbols):
	'''
	Checks configuration symbols and discovers the directories with
	SConscripts that needs to be compiled for those symbols.
	Also derives symbols and adds them to CPPFLAGS.
	'''
	archdir = None
	subarchdir = None
	platdir = None
	driversdirs = []
	for sym in config_symbols:
		for arch in arch_to_dir:
			if sym == arch:
				archdir = arch_to_dir[arch]
				env = arch_to_env[arch]
				Export('env')
				if arch == 'ARCH_TEST':
					env.Append(CPPFLAGS = '-DSUBARCH_TEST ')
					env.Append(CPPFLAGS = '-DPLATFORM_TEST ')
					subarchdir = "tests"
					platdir = "tests"
		for subarch in subarch_to_dir:
			if sym == subarch:
				subarchdir = subarch_to_dir[subarch]
		for plat in plat_to_dir:
			if sym == plat:
				platdir = plat_to_dir[plat]
		for driver in driver_to_dir:
			if sym == driver:
				# There can me multiple drivers, so making a list.
				driversdirs.append(driver_to_dir[driver])
	if archdir == None:
		print "Error: No config symbol found for architecture"
		sys.exit()
	if subarchdir == None:
		print "Error: No config symbol found for subarchitecture"
		sys.exit()
	if platdir == None:
		print "Error: No config symbol found for platform"
		sys.exit()

	# Update config.h with derived values. Unfortunately CML2 does not handle
	# derived symbols well yet. This can be fixed in the future.
	f = open(config_h, "a")
	f.seek(0, 2)
	f.write("/* Symbols derived from this file by SConstruct\process_config_symbols() */")
	f.write("\n#define __ARCH__\t\t%s\n" % archdir)
	f.write("#define __PLATFORM__\t\t%s\n" % platdir)
	f.write("#define __SUBARCH__\t\t%s\n\n" % subarchdir)
	f.close()
	return archdir, subarchdir, platdir, driversdirs


def process_config_symbols(config_symbols):
	'''
	Checks configuration symbols and discovers the directories with
	SConscripts that needs to be compiled for those symbols.
	'''
	archdir = None
	subarchdir = None
	platdir = None
	driversdirs = []
	for sym,val in config_symbols:
		if sym == "__ARCH__":
			archdir = val
			env = arch_to_env[val]
			Export('env')
		if sym == "__PLATFORM__":
			platdir = val
		if sym == "__SUBARCH__":
			subarchdir = val
		for driver in driver_to_dir:
			if sym == driver:
				# There can me multiple drivers, so making a list.
				driversdirs.append(driver_to_dir[driver])
	if archdir == None:
		print "Error: No config symbol found for architecture"
		sys.exit()
	if subarchdir == None:
		print "Error: No config symbol found for subarchitecture"
		sys.exit()
	if platdir == None:
		print "Error: No config symbol found for platform"
		sys.exit()

	return archdir, subarchdir, platdir, driversdirs

def gather_config_symbols(header_file):
	'''
	Gathers configuration symbols from config.h to be used in the build. This
	is particularly used for determining what sources to build. Each Sconscript
	has the option to select sources to build based on the symbols imported to it.
	'''
	if not os.path.exists(header_file):
		print "\n\%s does not exist. "\
		      "Please run: `scons configure' first\n\n" % header_file
		sys.exit()
	f = open(header_file)
	config_symbols = []
	while True:
		line = f.readline()
		if line == "":
			break
		str_parts = split(line)
		if len(str_parts) > 0:
			if str_parts[0] == "#define":
				config_symbols.append((str_parts[1], str_parts[2]))
	f.close()
	Export('config_symbols')
	return config_symbols

def gather_sconscripts(config_symbols):
	"Gather all SConscripts to be compiled depending on configuration symbols"
	arch, subarch, plat, drivers = process_config_symbols(config_symbols)
	dirpath = []
	allpaths = []

	# Paths for the SConscripts: sc_<path> etc. glob.glob is useful in that
	# it returns `None' for non-existing paths.
	sc_generic = glob.glob("src/generic/SConscript")
	sc_api = glob.glob("src/api/SConscript")
	sc_lib = glob.glob("src/lib/SConscript")
	sc_platform = glob.glob("src/platform/" + plat + "/SConscript")
	sc_arch = glob.glob("src/arch/" + arch + "/SConscript")
	sc_glue = glob.glob("src/glue/" + arch + "/SConscript")
	sc_subarch = glob.glob("src/arch/" + arch + "/" + subarch + "/SConscript")
	sc_drivers = []
	for driver in drivers:
		sc_drivers.append("src/drivers/" + driver + "/SConscript")
	#print "\nSConscripts collected for this build:"
	for dirpath in [sc_generic, sc_api, sc_lib, sc_platform, sc_arch, sc_subarch, sc_glue]:
		for path in dirpath:
	#		print path
			allpaths.append(join(os.getcwd(), path[:-11]))
	for drvpath in sc_drivers:
		allpaths.append(join(os.getcwd(), drvpath[:-11]))
	return allpaths

def gather_kernel_build_paths(scons_script_path_list):
	'''
	Takes the list of all SConscript files, and returns the list of
	corresponding build directories for each SConscript file.
	'''
	build_path_list = []
	for path in scons_script_path_list:
		build_path_list.append('build' + path)
	return build_path_list

def declare_kernel_build_paths(script_paths):
	'''
	Gathers all SConscript files and their corresponding
	distinct build directories. Declares the association
	to the scons build system.
	'''
	# Get all the script and corresponding build paths
	build_paths = gather_kernel_build_paths(script_paths)
	script_paths.sort()
	build_paths.sort()

	# Declare build paths
	for script_path, build_path in zip(script_paths, build_paths):
#		print "Sources in " + script_path + " will be built in: " + build_path
		BuildDir(build_path, script_path)

	return build_paths

def build_and_gather(bpath_list):
	'''
	Given a SConscript build path list, declares all
	SConscript files and builds them, gathers and
	returns the resulting object files.
	'''
	obj_list = []
	for path in bpath_list:
#		print "Building files in: " + path
		o = SConscript(path + '/' + 'SConscript')
		obj_list.append(o)
	return obj_list

def determine_target(config_symbols):
	if "ARCH_TEST" in config_symbols:
		target = "tests"
	else:
		target = "kernel"
	return target

def build_kernel_objs(config_symbols):
	"Builds the final kernel executable."
	# Declare builddir <--> SConscript association
	script_paths = gather_sconscripts(config_symbols)
	build_paths = declare_kernel_build_paths(script_paths)
	# Declare Sconscript files, build and gather objects.
	objs = build_and_gather(build_paths)
	return objs

#################################################################
# Kernel and Test Targets					#
# Executes python functions and compiles the kernel.		#
#################################################################

# Unfortunately the build system is imperative rather than declarative,
# and this is the entry point. This inconvenience is due to the fact that
# it is not easily possible in SCons to produce a target list dynamically,
# which, is generated by the `configure' target.

if "build" in COMMAND_LINE_TARGETS:
	config_symbols = gather_config_symbols(config_h)
	target = determine_target(config_symbols)
	if target == "kernel":
		built_objs = build_kernel_objs(config_symbols)
		kernel_target = kernel_env.Program('build/start', built_objs)
		kernel_env.Alias('build', kernel_target)
	elif target == "tests":
		built_objs = build_kernel_objs(config_symbols)
		tests_target = tests_env.Program('build/tests', built_objs)
		tests_env.Alias('build', tests_target)
	else:
		print "Invalid or unknown target.\n"

#################################################################
# The Configuration Targets:					#
# Executes python functions/commands to configure the kernel.	#
#################################################################
#  Configuration Build Details:					#
#  ----------------------------					#
#  1) Compiles the ruleset for given architecture/platform.	#
#  2) Invokes the CML2 kernel configurator using the given 	#
#  ruleset.							#
#  3) Configurator output is translated into a header file.	#
#  								#
#  For more information on this see the cml2 manual. Recap:	#
#  The cml file defines the rules (i.e. option a and b		#
#  implies c etc.) The compiler converts it to a pickled	#
#  form. The configurator reads it and brings up a UI menu.	#
#  Saving the selections results in a configuration file,	#
#  which in turn is converted to a header file includable	#
#  by the C sources.						#
#################################################################

def cml_configure(target, source, env):

	default_target = 'config.out'
	default_source = 'rules.out'

	target = str(target[0])
	source = str(source[0])
	# Strangely, cmlconfigure.py's command line option parsing is fairly broken
	# if you specify an input or output file a combination of things can happen,
	# so we supply no options, and move the defaults ifile= ./rules.out
	# ofile=./config.out around to match with supplied target/source arguments.

	# Move source to where cmlconfigure.py expects to find it.
	print "Moving", source, "to", default_source
	shutil.move(source, default_source)
	cml2_configure_cmd = cml2_tools_root + '/cmlconfigure.py -c'
	os.system(cml2_configure_cmd)

	# Move target file to where the build system expects to find it.
	print "Moving", default_target, "to", target
	shutil.move(default_target, target)
	print "Moving", default_source, "back to", source
	shutil.move(default_source, source)
	return None

def cml_compile(target, source, env):
	# Same here, this always produces a default file.
	default_target = "rules.out"

	target = str(target[0])
	source = str(source[0])

	cml2_compile_cmd = cml2_tools_root + '/cmlcompile.py < ' + source + ' > ' + target
	os.system(cml2_compile_cmd)

	print "Moving " + default_target + " to " + target
	shutil.move(default_target, target)
	return None

def derive_symbols(config_header):
	'''
	Reads target header (config.h), derives symbols, and appends them to the same file.
	Normally CML2 should do this on-the-fly but, doesn't so this function explicitly
	does it here.
	'''
	archdir = None
	subarchdir = None
	platdir = None
	driversdirs = []

	config_symbols = gather_config_symbols(config_header)
	for sym,val in config_symbols:
		for arch in arch_to_dir:
			if sym == arch:
				archdir = arch_to_dir[arch]
				if arch == "ARCH_TEST":
		# These are derived from ARCH_TEST, but don't need to be written,
		# because they are unused. Instead, __ARCH__ test etc. are derived
		# from these and written to config.h because those are used by the
		# build system. IOW these are transitional symbols.
					config_symbols.append(("TEST_SUBARCH","1"))
					config_symbols.append(("TEST_PLATFORM","1"))
		for subarch in subarch_to_dir:
			if sym == subarch:
				subarchdir = subarch_to_dir[subarch]
		for plat in plat_to_dir:
			if sym == plat:
				platdir = plat_to_dir[plat]
		for driver in driver_to_dir:
			if sym == driver:
				# There can me multiple drivers, so making a list.
				driversdirs.append(driver_to_dir[driver])
	if archdir == None:
		print "Error: No config symbol found for architecture"
		sys.exit()
	if subarchdir == None:
		print "Error: No config symbol found for subarchitecture"
		sys.exit()
	if platdir == None:
		print "Error: No config symbol found for platform"
		sys.exit()

	# Update config.h with derived values. Unfortunately CML2 does not handle
	# derived symbols well yet. This can be fixed in the future.
	f = open(config_h, "a")
	f.seek(0, 2)
	f.write("/* Symbols derived from this file by SConstruct\derive_symbols() */")
	f.write("\n#define __ARCH__\t\t%s\n" % archdir)
	f.write("#define __PLATFORM__\t\t%s\n" % platdir)
	f.write("#define __SUBARCH__\t\t%s\n\n" % subarchdir)

	f.close()


def cml_config2header(target, source, env):
	'''
	Translate the output of cml2 configurator to a C header file,
	to be included by the kernel.
	'''
	target = str(target[0])
	source = str(source[0])
	config_translator = cml2header.cml2header_translator()
	if config_translator.translate(source, target) < 0:
		print 'Configuration translation failed.'
	##
	##	CML2 is incapable of deriving symbols. Hence, derived
	##	symbols are handled here.
	derive_symbols(target)

# `hash' ('#') is a shorthand meaning `relative to
# the root directory'. But this only works for
# well-defined variables, not all paths, and only
# if we use a relative build directory for sources.
# Individual Build commands for configuration targets

cml_compiled_target = kernel_env.Command('#build/rules.out', '#configs/arm.cml', cml_compile)
cml_configured_target = kernel_env.Command('#build/config.out', '#build/rules.out', cml_configure)
cml_translated_target = kernel_env.Command(config_h, '#build/config.out', cml_config2header)

if "configure" in COMMAND_LINE_TARGETS:
	# This ensures each time `scons configure' is typed, a configuration occurs.
	# Otherwise the build system would think target is up-to-date and do nothing.
	kernel_env.AlwaysBuild(cml_compiled_target)
	kernel_env.AlwaysBuild(cml_configured_target)
	kernel_env.AlwaysBuild(cml_translated_target)

# Commandline aliases for each individual configuration targets
kernel_env.Alias('cmlcompile', cml_compiled_target)
kernel_env.Alias('cmlconfigure', cml_configured_target)
kernel_env.Alias('cmltranslate', cml_translated_target)

# Below alias is enough to trigger all config dependency tree.
kernel_env.Alias('configure', cml_translated_target)

