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
CONFIG_H = join("include/l4/config.h")
CONFIG_SHELVE_DIR = join(BUILDDIR, "configdata")
CONFIG_SHELVE_FILENAME = "configuration"
CONFIG_SHELVE = join(CONFIG_SHELVE_DIR, CONFIG_SHELVE_FILENAME)
configuration = {}

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
            self.arch = parts[2].lower()

    # Extract subarch from a name value pair
    def get_subarch(self, name, val):
        if name[:len("CONFIG_SUBARCH_")] == "CONFIG_SUBARCH_":
            parts = name.split("_", 3)
            self.subarch = parts[2].lower()

    # Extract platform from a name value pair
    def get_platform(self, name, val):
        if name[:len("CONFIG_PLATFORM_")] == "CONFIG_PLATFORM_":
            parts = name.split("_", 3)
            self.platform = parts[2].lower()

    # Extract number of containers
    def get_ncontainers(self, name, val):
        if name[:len("CONFIG_CONTAINERS")] == "CONFIG_CONTAINERS":
            self.ncontainers = val

symbols = config_symbols()

def cml2_header_to_symbols(cml2_header):
    with file(cml2_header) as header_file:
        for line in header_file:
            pair = symbols.line_to_name_value(line)
            if pair is not None:
                name, value = pair
                symbols.get_all(name, value)
                symbols.get_arch(name, value)
                symbols.get_subarch(name, value)
                symbols.get_platform(name, value)
                symbols.get_ncontainers(name, value)

def cml2_update_config_h(symbols, config_h_path):
    with open(config_h_path, "a") as config_h:
        config_h.write("#define __ARCH__ " + symbols.arch + '\n')
        config_h.write("#define __PLATFORM__ " + symbols.platform + '\n')
        config_h.write("#define __SUBARCH__ " + symbols.subarch + '\n')

def cml2_configure(cml2_config_file):
    os.system(CML2TOOLSDIR + '/cmlcompile.py -o ' + CML2RULES + ' ' + cml2_config_file)
    os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + CML2_CONFIG_PROPERTIES + ' ' + CML2RULES)
    os.system(TOOLSDIR + '/cml2header.py -o ' + CML2_CONFIG_H + ' -i ' + CML2_CONFIG_PROPERTIES)
    if not os.path.exists("build/l4"):
        os.mkdir("build/l4")
    shutil.copy(CML2_CONFIG_H, CONFIG_H)

def save_configuration():
    if not os.path.exists(CONFIG_SHELVE_DIR):
        os.mkdir(CONFIG_SHELVE_DIR)

    config_shelve = shelve.open(CONFIG_SHELVE)
    #config_shelve["config_symbols"] = symbols
    config_shelve["arch"] = symbols.arch
    config_shelve["subarch"] = symbols.subarch
    config_shelve["platform"] = symbols.platform
    config_shelve["all_symbols"] = symbols.all
    config_shelve.close()

def configure_kernel(cml_file):
    if not os.path.exists(BUILDDIR):
        os.mkdir(BUILDDIR)

    cml2_configure(cml_file)
    cml2_header_to_symbols(CML2_CONFIG_H)
    cml2_update_config_h(symbols, CONFIG_H)
    save_configuration()

if __name__ == "__main__":
    configure_kernel("configs/arm.cml")
