#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- Virtualization microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys, shelve, glob
from os.path import join

PROJRELROOT = '../../'

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))
sys.path.append(os.path.abspath("../"))

SCRIPTROOT = os.path.abspath(os.path.dirname(__file__))

from config.projpaths import *
from config.configuration import *

class BareContGenerator:
    def __init__(self):
        self.CONT_SRC_DIR = ''   # Set when container is selected
        self.BARE_SRC_BASEDIR = join(PROJROOT, 'conts')

        self.build_script_in = join(SCRIPTROOT, 'SConstruct.in')
        self.build_readme_in = join(SCRIPTROOT, 'build.readme.in')
        self.build_desc_in = join(SCRIPTROOT, 'container.desc.in')

        self.build_script_name = 'SConstruct'
        self.build_readme_name = 'build.readme'
        self.build_desc_name = 'container.description'

        self.build_script_out = None
        self.build_readme_out = None
        self.build_desc_out = None
        self.main_builder_name = 'build.py'
        self.main_configurator_name = 'configure.py'
        self.mailing_list_url = 'http://lists.l4dev.org/mailman/listinfo/codezero-devel'

    def create_bare_dirtree(self, config, cont):
        # First, create the container directory structure
        os.mkdir(self.CONT_SRC_DIR)
        os.mkdir(join(self.CONT_SRC_DIR, 'src'))
        os.mkdir(join(self.CONT_SRC_DIR, 'include'))

    def copy_bare_build_desc(self, config, cont):
        with open(self.build_desc_in) as fin:
            str = fin.read()
            with open(self.build_desc_out, 'w+') as fout:
                # Make any manipulations here
                fout.write(str)

    def copy_bare_build_readme(self, config, cont):
        with open(self.build_readme_in) as fin:
            str = fin.read()
            with open(self.build_readme_out, 'w+') as fout:
                # Make any manipulations here
                fout.write(str % (self.mailing_list_url, \
                                  cont.name, \
                                  self.build_desc_name, \
                                  self.main_builder_name, \
                                  self.main_configurator_name, \
                                  self.main_configurator_name))

    def copy_bare_build_script(self, config, cont):
        with open(self.build_script_in) as fin:
            str = fin.read()
            with open(self.build_script_out, 'w+') as fout:
                # Make any manipulations here
                fout.write(str)

    def create_bare_sources(self, config, cont):
        # Determine container directory name.
        self.CONT_SRC_DIR = join(self.BARE_SRC_BASEDIR, cont.dirname.lower())
        print "container source dir: " + self.CONT_SRC_DIR

        self.build_script_out = join(self.CONT_SRC_DIR, self.build_script_name)
        self.build_readme_out = join(self.CONT_SRC_DIR, self.build_readme_name)
        self.build_desc_out = join(self.CONT_SRC_DIR, self.build_desc_name)

        self.create_bare_dirtree(config, cont)
        self.copy_bare_build_script(config, cont)
        self.copy_bare_build_readme(config, cont)
        self.copy_bare_build_desc(config, cont)
        #copy_bare_sources(config, cont)

    def check_create_bare_sources(self, config):
        for cont in config.containers:
            if cont.type == "bare":
                print "Cont type: " + cont.type
                if not os.path.exists(join(self.BARE_SRC_BASEDIR, cont.dirname)):
                    print "Creating new at " + join(self.BARE_SRC_BASEDIR, cont.dirname)
                    self.create_bare_sources(config, cont)

    def bare_container_generate(self, config):
        self.check_create_bare_sources(config)

if __name__ == "__main__":
    config = configuration_retrieve()
    bare_cont = BareContGenerator()
    bare_cont.bare_container_generate(config)

