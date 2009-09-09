#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-

import os
from os.path import join

PROJROOT = os.getcwd()
BUILDDIR = join(PROJROOT, "build")
TOOLSDIR = join(PROJROOT, "tools")
CML2TOOLSDIR = join(TOOLSDIR, "cml2-tools")
CML2RULES = join(BUILDDIR, "cml2_rules.out") 
CML2_CONFIG_PROPERTIES = join(BUILDDIR, "cml2_config.out")
CML2_CONFIG_H = join(BUILDDIR, "cml2_config.h")


configuration = {}

def cml2_rest():
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
    with open(config_h_path, "w+") as config_h:
        config_h.write("#define __ARCH__ " + configuration['ARCH'])
        config_h.write("#define __PLATFORM__" + configuration['PLATFORM'])
        config_h.write("#define __SUBARCH__" + configuration['SUBARCH'])
    print configuration

def cml2_process(cml2_conf_props):
    config_items = {}
    with file(cml2_conf_props) as config_file:
        for line in config_file:
            item = line.split('=')
            if len(item) == 2:
                config_items[item[0].strip()] = (item[1].strip() == 'y')
    return config_items

def cml2_configure(cml2_config_file):
    os.system(CML2TOOLSDIR + '/cmlcompile.py -o ' + CML2RULES + ' ' + cml2_config_file)
    os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + CML2_CONFIG_PROPERTIES + ' ' + CML2RULES)
    os.system(TOOLSDIR + '/cml2header.py -o ' + CML2_CONFIG_H + ' -i ' + CML2_CONFIG_PROPERTIES)

def main():
    cml2_configure("configs/arm.cml")
    cml2_rest()

if __name__ == "__main__":
    main()
