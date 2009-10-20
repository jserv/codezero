#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys, shelve, glob
from os.path import join

PROJRELROOT = '../../'

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))
sys.path.append(os.path.abspath("../"))

from config.projpaths import *
from config.configuration import *

containers_menu = \
'''
menu containers_menu
'''

containers_constraint = \
'''
unless CONTAINERS > %d suppress cont%d_menu
'''

containers_default = \
'''
default CONTAINERS from %d
'''

def add_container_constraint(cid):
    cml_string = ""
    if cid == 0:
        return ""
    cml_string = containers_constraint % (cid, cid)
    return cml_string

def generate_container_cml(arch, ncont):
    print "Autogenerating new rule file"
    fbody = ""
    with open(join(CML2_CONFIG_SRCDIR, arch + '.ruleset')) as in_ruleset:
        fbody += in_ruleset.read()

    # Add container visibility constraint
    for cont in range(ncont):
        fbody += add_container_constraint(cont)

    # Add number of default containers
    fbody += containers_default % ncont

    # Generate the containers menu with as many entries as containers
    fbody += containers_menu
    for cont in range(ncont):
        fbody += '\tcont%d_menu\n' % cont

    # Write each container's rules
    for cont in range(ncont):
        with open(CML2_CONT_DEFFILE, "rU") as contdefs:
            defbody = contdefs.read()
            defbody = defbody.replace("%\n", "%%\n")
            fbody += defbody % { 'cn' : cont }

    # Write the result to output rules file.
    with open(CML2_AUTOGEN_RULES, "w+") as out_cml:
        out_cml.write(fbody)
    return CML2_AUTOGEN_RULES

if __name__ == "__main__":
    generate_container_cml('arm', 4)

