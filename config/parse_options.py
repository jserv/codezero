#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
from optparse import OptionParser
import os, sys, shutil

PROJRELROOT = '../'

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))

from scripts.cml.generate_container_cml import *

# Parse options + autogenerate cml rule file if necessary.
def build_parse_options():
    autogen_true = 0
    usage = "usage: %prog [options] arg"
    parser = OptionParser(usage)

    parser.add_option("-a", "--arch", type = "string", dest = "arch",
                      help = "Use configuration file for architecture")
    parser.add_option("-n", "--num-containers", type = "int", dest = "ncont",
                      help = "Maximum number of containers that will be "
                             "made available in configuration")
    parser.add_option("-c", "--configure-first", action = "store_true", dest = "config",
                      help = "Tells the build script to run configurator first")
    parser.add_option("-f", "--use-file", dest = "cml_file",
                      help = "Supply user-defined cml file "
                             "(Use only if you want to override default)")
    parser.add_option("-r", "--reset-old-config", action = "store_true",
                      default = False, dest = "reset_old_config",
                      help = "Reset configuration file settings "
                             "(If you had configured before and changing the "
                             "rule file, this will reset existing values to default)")
    parser.add_option("-s", "--save-old-config", action = "store_true",
                      default = False, dest = "backup_config",
                      help = "Backs up old configuration file settings to a .saved file"
                             "(Subsequent calls would overwrite. Only meaningful with -r)")
    parser.add_option("-p", "--print-config", action = "store_true",
                      default = False, dest = "print_config",
                      help = "Prints out configuration settings"
                             "(Symbol values and container parameters are printed)")


    (options, args) = parser.parse_args()

    autogen_true = options.backup_config or options.reset_old_config \
                   or options.cml_file or options.ncont \
                   or options.arch or not os.path.exists(CML2_CML_FILE)

    # Prepare default if arch not supplied
    if autogen_true and not options.arch:
        print "No arch supplied (-a), using `arm' as default."
        options.arch = "arm"

    # Prepare default if number of containers not supplied
    if autogen_true and not options.ncont:
        options.ncont = 4
        print "Max container count not supplied (-n), using %d as default." % options.ncont

    # Regenerate cml file if options are supplied or the file doesn't exist.
    if autogen_true:
        generate_container_cml(options.arch, options.ncont)
        if options.reset_old_config == 1 and os.path.exists(CML2_OLDCONFIG_FILE):
            if options.backup_config == 1:
                print "Backing up %s into %s" % (CML2_OLDCONFIG_FILE, CML2_OLDCONFIG_FILE + '.saved')
                shutil.move(CML2_OLDCONFIG_FILE, CML2_OLDCONFIG_FILE + '.saved')
            else:
                print "Deleting %s" % CML2_OLDCONFIG_FILE
                os.remove(CML2_OLDCONFIG_FILE)
    return options, args
