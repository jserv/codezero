#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
# Top-level build script for Codezero
#
# Configures the Codezero environment, builds the kernel and userspace
# libraries, builds and packs all containers and builds the final loader
# image that contains all images.
#
import os, sys, shelve, shutil
from os.path import join
from config.projpaths import *
from config.configuration import *
from scripts.conts import containers
from configure import *
from optparse import OptionParser
from scripts.cml.generate_container_cml import *

# NOTE:
# The scripts obtain all the configuration data (a set of class
# instances) from the configuration shelve, so we don't pass
# any arguments here.

def main():
    autogen_true = 0
    usage = "usage: %prog [options] arg"
    parser = OptionParser(usage)

    parser.add_option("-a", "--arch", type = "string", dest = "arch",
                      help = "Use configuration file for architecture")
    parser.add_option("-c", "--num-containers", type = "int", dest = "ncont",
                      help = "Maximum number of containers available in configuration")
    parser.add_option("-f", "--use-file", dest = "cml_file",
                      help = "Supply user-defined cml file"
                             "(Use only if you want to override default)")
    parser.add_option("-w", "--wipeout-old-config", action = "store_true",
                      default = False, dest = "wipeout",
                      help = "Wipe out existing configuration file settings")

    (options, args) = parser.parse_args()

    autogen_true = len(sys.argv) > 1 or not os.path.exists(CML2_CML_FILE)

    if autogen_true and not options.arch:
        print "No arch supplied (-a), using `arm' as default."
        options.arch = "arm"
    if autogen_true and not options.ncont:
        options.ncont = 4
        print "Max container count not supplied (-c), using %d as default." % options.ncont

    # Regenerate cml file if options are supplied or the file doesn't exist.
    if autogen_true:
        generate_container_cml(options.arch, options.ncont)
        if options.wipeout == 1 and os.path.exists(CML2_OLDCONFIG_FILE):
            print "Deleting %s" % CML2_OLDCONFIG_FILE
            os.remove(CML2_OLDCONFIG_FILE)

    #
    # Configure
    #
    configure_kernel(CML2_CML_FILE)

    #
    # Build the kernel
    #
    print "\nBuilding the kernel..."
    os.chdir(PROJROOT)
    os.system("scons")

    #
    # Build userspace libraries
    #
    print "\nBuilding userspace libraries..."
    os.system('scons -f SConstruct.userlibs')

    #
    # Build containers
    #
    print "\nBuilding containers..."
    containers.build_all_containers()

    #
    # Build libs and loader
    #
    os.chdir(PROJROOT)
    print "\nBuilding the loader and packing..."
    os.system("scons -f SConstruct.loader")

if __name__ == "__main__":
    main()
