#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd

import os, sys, shelve, shutil
from os.path import join

PROJRELROOT = "../.."
SCRIPTROOT = os.path.abspath(os.path.dirname("."))
sys.path.append(os.path.abspath(PROJRELROOT))

from config.projpaths import *
from config.configuration import *

LINUX_ROOTFS_BUILDDIR = join(BUILDDIR, os.path.relpath(LINUX_ROOTFSDIR, PROJROOT))

rootfs_lds_in = join(LINUX_ROOTFSDIR, "rootfs.lds.in")
rootfs_lds_out = join(LINUX_ROOTFS_BUILDDIR, "rootfs.lds")
rootfs_elf_out = join(LINUX_ROOTFS_BUILDDIR, "rootfs.elf")

def main():
    os.chdir(LINUX_ROOTFSDIR)
    config_symbols = configuration_retrieve()
    if not os.path.exists(LINUX_ROOTFS_BUILDDIR):
        os.makedirs(LINUX_ROOTFS_BUILDDIR)
    os.system("arm-none-linux-gnueabi-cpp -P " + \
              "%s > %s" % (rootfs_lds_in, rootfs_lds_out))
    os.system("arm-none-linux-gnueabi-gcc " + \
              "-nostdlib -o %s -T%s rootfs.S" % (rootfs_elf_out, rootfs_lds_out))

def clean():
    if os.path.exists(LINUX_ROOTFS_BUILDDIR):
        shutil.rmtree(LINUX_ROOTFS_BUILDDIR)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        main()
    elif "clean" == sys.argv[1]:
        clean()
    else:
        print " Usage: %s [clean]" % (sys.argv[0])
