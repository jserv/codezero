#! /usr/bin/env python2.7
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
from scripts.config.projpaths import *
from scripts.config.configuration import *
from scripts.config.config_check import *
from scripts.qemu.qemu_cmdline import *
from scripts.conts import containers
from scripts.config.config_invoke import *
from scripts.kernel.check_kernel_limit import *

def build_system(opts, args):
    if opts.print_config:
        config = configuration_retrieve()
        if config:
            config.config_print()
        else:
            print '\nNo configuration to print...\n'
        return None

    # Configure
    configure_system(opts, args)

    # Are we asked to configure only
    if opts.config_only:
        return None

    # Check for sanity of containers
    sanity_check_conts()

    # Build userspace libraries
    os.chdir(USERLIBS_DIR)
    print "\nBuilding userspace libraries..."
    ret = os.system("scons" + " -j " + opts.jobs)
    if(ret):
        print "Building userspace libraries failed...\n"
        sys.exit(1)

    # Build containers
    os.chdir(PROJROOT)
    print "\nBuilding containers..."
    containers.build_all_containers(opts)

    # Build the kernel
    print "\nBuilding the kernel..."
    os.chdir(PROJROOT)
    ret = os.system("scons" + " -j " + opts.jobs)
    if(ret):
        print "Building kernel failed...\n"
        sys.exit(1)

    # Check if kernel overlap with first container
    if(check_kernel_container_overlap()):
        print "Kernel overlaps with first continer."
        print "Please shift the first container to some higher " + \
              "address after the kernel and build again...\n"
        sys.exit(1)

    # Build the loader
    os.chdir(PROJROOT)
    print "\nBuilding the loader and packing..."
    ret = os.system("scons -f " + join(LOADERDIR, 'SConstruct') + " -j " + opts.jobs)
    if(ret):
        print "Building loader failed...\n"
        sys.exit(1)

    # Build qemu-insight-script
    print "\nBuilding qemu-insight-script..."
    build_qemu_cmdline_script()

    print "\nBuild complete..."
    print "\nRun qemu with following command: ./tools/run-qemu-insight\n"

    renamed_cml = rename_config_cml(opts)
    if(renamed_cml):
        print 'CML configuration file saved as ' + renamed_cml + '\n'

    return None

def clean_system(opts):
    # Clean only if some config exists.
    if not configuration_retrieve():
        print '\nNo configuration exists, nothing to clean..\n'
        return None

    # Clean userspace libraries
    os.chdir(USERLIBS_DIR)
    print "\nCleaning userspace libraries..."
    ret = os.system("scons -c")

    # Clean containers
    os.chdir(PROJROOT)
    print "\nCleaning containers..."
    containers.clean_all_containers()

    # Clean the kernel
    print "\nCleaning the kernel..."
    os.chdir(PROJROOT)
    ret = os.system("scons -c")

    # Clean the loader
    os.chdir(PROJROOT)
    print "\nCleaning the loader..."
    ret = os.system("scons -c -f " + join(LOADERDIR, 'SConstruct'))

    # Remove qemu-insight-script
    print "\nRemoving qemu-insight-script..."
    clean_qemu_cmdline_script()

    if opts.clean_all:
        cml_files_cleanup()

    print '\nCleaning Done...\n'
    return None

def main():
    opts, args = build_parse_options()

    if opts.clean or opts.clean_all:
        clean_system(opts)
    else:
        build_system(opts, args)

    return None

if __name__ == "__main__":
    main()
