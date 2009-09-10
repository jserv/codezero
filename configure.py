#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-

import os, sys, shelve, shutil
from os.path import join

PROJROOT = os.getcwd()
BUILDDIR = join(PROJROOT, "build")
TOOLSDIR = join(PROJROOT, "tools")
CML2TOOLSDIR = join(TOOLSDIR, "cml2-tools")
CML2RULES = join(BUILDDIR, "cml2_rules.out")
CML2_CONFIG_PROPERTIES = join(BUILDDIR, "cml2_config.out")
CML2_CONFIG_H = join(BUILDDIR, "cml2_config.h")
CONFIG_H = join(BUILDDIR, "l4/config.h")
CONFIG_DATA_DIR = join(BUILDDIR, "configdata")
CONFIG_DATA_FILENAME = "configuration"
CONFIG_DATA = join(CONFIG_DATA_DIR, CONFIG_DATA_FILENAME)
configuration = {}
config_data = {}

def save_configuration(configuration):
    if not os.path.exists(CONFIG_DATA_DIR):
        os.mkdir(CONFIG_DATA_DIR)

    config_shelve = shelve.open(CONFIG_DATA)
    config_shelve["cml2_config"] = configuration
    config_shelve.close()

'''
def cml2_parse_config_symbol(name, value):
     sname, sval = line.split('=')
        if len(config_sym_pair) == 2: # This means it is a valid symbol
            cml2_parse_config_symbol(name, value)
    config, rest = name.split("_", 1)
    print symparts
'''

class config_symbols:
    arch = None
    subarch = None
    platform = None
    kbuild = []
    all = []

    # Get all name value symbols
    def get_all(self, name, val):
        self.all.append([name, val])

    # Convert line to name value pair, if possible
    def line_to_name_value(self, line):
        parts = line.split()
        if len(parts) > 0:
            if parts[0] == "#define":
                return parts[1], parts[2]
        return None

    # Extract architecture from a name value pair
    def get_arch(self, name, val):
        if name[:len("CONFIG_ARCH_")] == "CONFIG_ARCH_":
            parts = name.split("_", 3)
            print parts
            self.arch = parts[2].lower()


symbols = config_symbols()

def cml2_header_to_symbols(cml2_header):
    with file(cml2_header) as header_file:
        for line in header_file:
            pair = symbols.line_to_name_value(line)
            if pair is not None:
                print pair
                name, value = pair
                symbols.get_all(name,value)
                symbols.get_arch(name,value)
    print symbols.all
    print symbols.arch

def cml2_update_config_h(configuration):
    config_h_path = BUILDDIR + '/l4/config.h'
    with open(config_h_path, "a") as config_h:
        config_h.write("#define __ARCH__ " + configuration['ARCH'] + '\n')
        config_h.write("#define __PLATFORM__ " + configuration['PLATFORM'] + '\n')
        config_h.write("#define __SUBARCH__ " + configuration['SUBARCH'] + '\n')

def cml2_configure(cml2_config_file):
    os.system(CML2TOOLSDIR + '/cmlcompile.py -o ' + CML2RULES + ' ' + cml2_config_file)
    os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + CML2_CONFIG_PROPERTIES + ' ' + CML2RULES)
    os.system(TOOLSDIR + '/cml2header.py -o ' + CML2_CONFIG_H + ' -i ' + CML2_CONFIG_PROPERTIES)
    if not os.path.exists("build/l4"):
        os.mkdir("build/l4")
    shutil.copy(CML2_CONFIG_H, CONFIG_H)

def configure_kernel(cml_file):
    if not os.path.exists(BUILDDIR):
        os.mkdir(BUILDDIR)

    cml2_configure(cml_file)
    cml2_header_to_symbols(CML2_CONFIG_H)
    #cml2_parse_config_data(configuration, config_data)
    #cml2_update_config_h(configuration)
    save_configuration(configuration)

    return configuration

if __name__ == "__main__":
    configure_kernel("configs/arm.cml")
