#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-

import os, sys, shelve, shutil
from os.path import join
from config.projpaths import *
from config.configuration import *
from scripts.bare.bare_generator import *
from scripts.conts.generate_kernel_cinfo import *
from config.parse_options import *

def cml2_header_to_symbols(cml2_header, config):
    with file(cml2_header) as header_file:
        for line in header_file:
            pair = config.line_to_name_value(line)
            if pair is not None:
                name, value = pair
                config.get_all(name, value)
                config.get_arch(name, value)
                config.get_subarch(name, value)
                config.get_platform(name, value)
                config.get_ncontainers(name, value)
                config.get_container_parameters(name, value)

def cml2_update_config_h(config_h_path, config):
    with open(config_h_path, "a") as config_h:
        config_h.write("#define __ARCH__ " + config.arch + '\n')
        config_h.write("#define __PLATFORM__ " + config.platform + '\n')
        config_h.write("#define __SUBARCH__ " + config.subarch + '\n')

def cml2_configure(cml2_config_file):
    os.system(CML2TOOLSDIR + '/cmlcompile.py -o ' + CML2RULES + ' ' + cml2_config_file)
    if not os.path.exists(CML2_OLDCONFIG_FILE):
        os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + \
                  CML2_CONFIG_FILE + ' ' + CML2RULES)
    else:
        os.system(CML2TOOLSDIR + '/cmlconfigure.py -c -o ' + CML2_CONFIG_FILE + \
                  ' -i ' + CML2_OLDCONFIG_FILE +  ' ' + CML2RULES)
    os.system(TOOLSDIR + '/cml2header.py -o ' + CML2_CONFIG_H + ' -i ' + CML2_CONFIG_FILE)

    # Do the actual copying here
    shutil.copy(CML2_CONFIG_FILE, CML2_OLDCONFIG_FILE)

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

    # Generate bare container files if new ones defined
    bare_cont_gen = BareContGenerator()
    bare_cont_gen.bare_container_generate(config)

    # Generate kernel cinfo structure for container definitions
    generate_kernel_cinfo(config.containers, KERNEL_CINFO_PATH)

if __name__ == "__main__":
    build_parse_options()
    configure_kernel(join(CML2_CONFIG_SRCDIR, "out.cml"))

