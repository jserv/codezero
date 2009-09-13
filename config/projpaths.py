#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-

import os, sys, shelve, shutil
from os.path import join

# Way to get project root from any script importing this one :-)
PROJROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

BUILDDIR = join(PROJROOT, "build")
TOOLSDIR = join(PROJROOT, "tools")
CML2_CONFIG_SRCDIR = join(PROJROOT, "config/cml")
CML2TOOLSDIR = join(TOOLSDIR, "cml2-tools")
CML2RULES = join(BUILDDIR, "cml2_rules.out")
CML2_CONFIG_PROPERTIES = join(BUILDDIR, "cml2_config.out")
CML2_CONFIG_H = join(BUILDDIR, "cml2_config.h")
CONFIG_H = join("include/l4/config.h")
CONFIG_SHELVE_DIR = join(BUILDDIR, "configdata")
CONFIG_SHELVE_FILENAME = "configuration"
CONFIG_SHELVE = join(CONFIG_SHELVE_DIR, CONFIG_SHELVE_FILENAME)

LINUXDIR = join(PROJROOT, 'conts/linux')
LINUX_KERNELDIR = join(LINUXDIR, 'linux-2.6.28.10')
LINUX_ROOTFSDIR = join(LINUXDIR, 'rootfs')

