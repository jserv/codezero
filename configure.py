#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-

import os, sys, shelve, shutil
from os.path import join
from config.projpaths import *
from config.configuration import *


def cml2_header_to_symbols(cml2_header, symbols):
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
                symbols.get_container_parameters(name, value)

def cml2_update_config_h(config_h_path, config):
    with open(config_h_path, "a") as config_h:
        config_h.write("#define __ARCH__ " + config.arch + '\n')
        config_h.write("#define __PLATFORM__ " + config.platform + '\n')
        config_h.write("#define __SUBARCH__ " + config.subarch + '\n')

def cml2_configure(cml2_config_file):
    os.system(CML2TOOLSDIR + '/cmlcompile.py -o ' + CML2RULES + ' ' + cml2_config_file)
    os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + CML2_CONFIG_PROPERTIES + ' ' + CML2RULES)
    os.system(TOOLSDIR + '/cml2header.py -o ' + CML2_CONFIG_H + ' -i ' + CML2_CONFIG_PROPERTIES)
    if not os.path.exists("build/l4"):
        os.mkdir("build/l4")
    shutil.copy(CML2_CONFIG_H, CONFIG_H)

def configure_kernel(cml_file):
    config = configuration()

    if not os.path.exists(BUILDDIR):
        os.mkdir(BUILDDIR)

    cml2_configure(cml_file)
    cml2_header_to_symbols(CML2_CONFIG_H, config)
    cml2_update_config_h(CONFIG_H, config)
    configuration_save(config)
    #config.config_print()

if __name__ == "__main__":
    configure_kernel(join(CML2_CONFIG_SRCDIR, "arm.cml"))

