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

def save_configuration(configuration):
    if not os.path.exists(CONFIG_DATA_DIR):
        os.mkdir(CONFIG_DATA_DIR)

    config_shelve = shelve.open(CONFIG_DATA)
    config_shelve["cml2_config"] = configuration
    config_shelve.close()

def cml2_process(cml2_conf_props):
    config_items = {}
    with file(cml2_conf_props) as config_file:
        for line in config_file:
            item = line.split('=')
            if len(item) == 2:
                config_items[item[0].strip()] = (item[1].strip() == 'y')
    return config_items

def cml2_config_to_symbols():
    config_h_path = BUILDDIR + '/l4/config.h'
    config_data = cml2_process(CML2_CONFIG_PROPERTIES)

    for key, value in config_data.items():
        if value:
            items = key.split('_')
            if items[0] == 'ARCH':
                configuration['ARCH'] = items[1].lower()
    for key, value in config_data.items():
        if value:
            items = key.split('_')
            if items[0] == 'ARCH': continue
            if items[0] == configuration['ARCH'].upper():
                configuration[items[1]] = items[2].lower()
            else:
                path = items[1].lower() + '/' + items[2].lower()
                try:
                    configuration[items[0]]
                except KeyError:
                    configuration[items[0]] = []
                configuration[items[0]].append(path)
    with open(config_h_path, "a") as config_h:
        config_h.write("#define __ARCH__ " + configuration['ARCH'] + '\n')
        config_h.write("#define __PLATFORM__ " + configuration['PLATFORM'] + '\n')
        config_h.write("#define __SUBARCH__ " + configuration['SUBARCH'] + '\n')
    return configuration

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
    configuration = cml2_config_to_symbols()
    save_configuration(configuration)
    return configuration

if __name__ == "__main__":
    configure_kernel("configs/arm.cml")
