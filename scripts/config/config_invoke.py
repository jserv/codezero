#! /usr/bin/env python2.7
# -*- mode: python; coding: utf-8; -*-
import os, sys, shelve, shutil, time
from time import strftime, localtime
from os.path import join

from projpaths import *
sys.path.append(PROJROOT)
from configuration import *
from scripts.baremetal.baremetal_generator import *
from scripts.kernel.generate_kernel_cinfo import *
from scripts.cml.generate_container_cml import *
from optparse import OptionParser

#
# Rarely required. Only required when:
# The number of containers defined in the ruleset are not enough.
# E.g. you want 16 containers instead of 4.
# You want to start from scratch and set _all_ the parameters yourself.
#
def autogen_rules_file(options, args):
    # Prepare default if arch not supplied
    if not options.arch:
        print "No arch supplied (-a), using `arm' as default."
        options.arch = "arm"

    # Prepare default if number of containers not supplied
    if not options.ncont:
        options.ncont = 4
        print "Max container count not supplied (-n), using %d as default." % options.ncont

    return generate_container_cml(options.arch, options.ncont)


def cml2_header_to_symbols(cml2_header, config):
    with file(cml2_header) as header_file:
        for line in header_file:
            pair = config.line_to_name_value(line)
            if pair is not None:
                name, value = pair
                config.get_all(name, value)
                config.get_cpu(name, value)
                config.get_arch(name, value)
                config.get_subarch(name, value)
                config.get_platform(name, value)
                config.get_ncpu(name, value)
                config.get_ncontainers(name, value)
                config.get_container_parameters(name, value)
                config.get_toolchain(name, value)

def cml2_add_default_caps(config):
    for c in config.containers:
	create_default_capabilities(c)

def cml2_update_config_h(config_h_path, config):
    with open(config_h_path, "a") as config_h:
        config_h.write("#define __ARCH__ " + config.arch + '\n')
        config_h.write("#define __PLATFORM__ " + config.platform + '\n')
        config_h.write("#define __SUBARCH__ " + config.subarch + '\n')
        config_h.write("#define __CPU__ " + config.cpu + '\n')

def configure_kernel(cml_file):
    config = configuration()

    if not os.path.exists(BUILDDIR):
        os.mkdir(BUILDDIR)

    cml2_configure(cml_file)

# Parse options + autogenerate cml rule file if necessary.
def build_parse_options():
    autogen_true = 0
    usage = "\n\t%prog [options] arg\n\n\
             \r\t** :  Override all other options provided"
    parser = OptionParser(usage, version = "codezero 4.0")

    parser.add_option("-a", "--arch", type = "string", dest = "arch",
                      help = "Specify architecture.")

    parser.add_option("-n", "--num-containers", type = "int", dest = "ncont",
                      help = "Maximum number of containers supported in configuration.")

    parser.add_option("-f", "--file", dest = "cml_file",
                      help = "Specify user-defined configuration file.")

    parser.add_option("-C", "--configure", action = "store_true", dest = "config",
                      help = "Configure only.")

    parser.add_option("-r", "--reset-config", action = "store_true",
                      default = False, dest = "reset_config",
                      help = "Reset earlier configuration settings.")

    parser.add_option("-b", "--batch", action="store_true", dest="batch", default = False,
                      help = "Disable configuration screen, used with -f only.")

    parser.add_option("-j", "--jobs", type = "str", dest="jobs", default = "1",
                      help = "Enable parallel build with SCons and GNU/Make"
                             "try to launch more than one compilation at a time")

    parser.add_option("-p", "--print-config", action = "store_true",
                      default = False, dest = "print_config",
                      help = "Print configuration**")

    parser.add_option("-c", "--clean", action="store_true", dest="clean", default = False,
                      help = "Do cleanup excluding configuration files**")

    parser.add_option("-x", "--clean-all", action="store_true", dest="clean_all", default = False,
                      help = "Do cleanup including configuration files**")

    (options, args) = parser.parse_args()

    if options.cml_file and options.reset_config:
        parser.error("options -f and -r are mutually exclusive")
        exit()

    # if -C, configure only.
    # -f or -r or -n or -a implies -C.
    options.config_only = 0
    if options.config:
        options.config_only = 1
    elif options.cml_file or options.ncont or options.arch or options.reset_config \
       or not os.path.exists(BUILDDIR) or not os.path.exists(CONFIG_SHELVE_DIR):
        options.config = 1

    return options, args


def configure_system(options, args):
    #
    # Configure only if we are told to do so.
    #
    if not options.config:
        return

    if not os.path.exists(BUILDDIR):
        os.mkdir(BUILDDIR)

    #
    # If we have an existing config file or one supplied in options
    # and we're not forced to autogenerate, we use the config file.
    #
    # Otherwise we autogenerate a ruleset and compile it, and create
    # a configuration file from it from scratch.
    #
    if (options.cml_file or os.path.exists(CML2_CONFIG_FILE)) \
            and not options.reset_config:
        if options.cml_file:
            cml2_config_file = options.cml_file
        else:
            cml2_config_file = CML2_CONFIG_FILE

        #
        # If we have a valid config file but not a rules file,
        # we still need to autogenerate the rules file.
        #
        if not os.path.exists(CML2_COMPILED_RULES):
            rules_file = autogen_rules_file(options, args)

            # Compile rules file.
            os.system(CML2TOOLSDIR + '/cmlcompile.py -o ' + \
                      CML2_COMPILED_RULES + ' ' + rules_file)

        if options.batch:
                # Create configuration from existing file
                os.system(CML2TOOLSDIR + '/cmlconfigure.py -b -o ' + \
                        CML2_CONFIG_FILE + ' -i ' + cml2_config_file + \
                        ' ' + CML2_COMPILED_RULES)
        else:
                # Create configuration from existing file
                os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + \
                        CML2_CONFIG_FILE + ' -i ' + cml2_config_file + \
                        ' ' + CML2_COMPILED_RULES)

    else:
        rules_file = autogen_rules_file(options, args)

        # Compile rules file.
        os.system(CML2TOOLSDIR + '/cmlcompile.py -o ' + \
                  CML2_COMPILED_RULES + ' ' + rules_file)

        # Create configuration from scratch
        os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + \
                  CML2_CONFIG_FILE + ' ' + CML2_COMPILED_RULES)

    # After configure, if user might have chosen to quit without saving
    if not os.path.exists(CML2_CONFIG_FILE):
        print "Exiting without saving configuration."
        sys.exit()

    # Create header file
    os.system(TOOLSDIR + '/cml2header.py -o ' + \
              CML2_CONFIG_H + ' -i ' + CML2_CONFIG_FILE)

    # The rest:
    if not os.path.exists(os.path.dirname(CONFIG_H)):
        os.mkdir(os.path.dirname(CONFIG_H))

    shutil.copy(CML2_CONFIG_H, CONFIG_H)

    config = configuration()
    cml2_header_to_symbols(CML2_CONFIG_H, config)
    cml2_add_default_caps(config)
    cml2_update_config_h(CONFIG_H, config)

    configuration_save(config)

    # Initialise config dependent projpaths
    define_config_dependent_projpaths(config)

    # Generate baremetal container files if new ones defined
    baremetal_cont_gen = BaremetalContGenerator()
    baremetal_cont_gen.baremetal_container_generate(config)

    return config
#
# Rename config.cml to more human friendly name
# Name of new cml is based on paltform and
# containers configured in the system.
#
def rename_config_cml(options):
    if not options.config:
        return None

    config = configuration_retrieve()
    new_cml_file = \
        join(BUILDDIR, strftime("%a-%d%b%Y-%H:%M:%S", localtime()) + \
                       '-' + config.platform)
    for cont in config.containers:
        new_cml_file += '-' + cont.name
        new_cml_file += '.cml'

    os.system("cp -f " + CML2_CONFIG_FILE + " " + new_cml_file)
    return new_cml_file

def cml_files_cleanup():
    '''
    config = configuration_retrieve()
    new_cml_file = join(BUILDDIR, config.platform)
    for cont in config.containers:
        new_cml_file += '-' + cont.name
    new_cml_file += '.cml'

    #os.system("rm -f " + CML2_CONFIG_FILE)
    #os.system("rm -f " + new_cml_file)
    #os.system("rm -f " + CML2_COMPILED_RULES)
    #os.system("rm -f " + CML2_CONFIG_H)
    #os.system("rm -f " + CML2_AUTOGEN_RULES)
    os.system("rm -f " + CONFIG_H)
    #os.system("rm -f " + CONFIG_SHELVE)
    '''
    os.system("rm -f " + CONFIG_H)
    os.system("rm -rf " + BUILDDIR)

    return None

if __name__ == "__main__":
    opts, args = build_parse_options()

    # We force configuration when calling this script
    # whereas build.py can provide it as an option
    opts.config = 1

    configure_system(opts, args)
    '''
    print '\nPlease use build.py for configuration and building.\n'
    '''
