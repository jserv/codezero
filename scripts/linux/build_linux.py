#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys, shelve
from os.path import join

PROJRELROOT = '../../'

SCRIPTROOT = os.path.abspath(os.path.dirname("."))
sys.path.append(os.path.abspath(PROJRELROOT))

from config.projpaths import *
from config.configuration import *

LINUX_KERNEL_BUILDDIR = join(BUILDDIR, os.path.relpath(LINUX_KERNELDIR, PROJROOT))

class LinuxBuilder:
    @staticmethod
    def build_linux(container):
        print '\nBuilding the linux kernel...'

        # Create linux kernel build directory path
        cont_builddir = join(BUILDDIR, "cont" + str(container.id))
        LINUX_KERNEL_BUILDDIR = join(cont_builddir, \
                                     os.path.relpath(LINUX_KERNELDIR, \
                                                     PROJROOT))

        os.chdir(LINUX_KERNELDIR)
        if not os.path.exists(LINUX_KERNEL_BUILDDIR):
            os.makedirs(LINUX_KERNEL_BUILDDIR)
        os.system("make defconfig ARCH=arm O=" + LINUX_KERNEL_BUILDDIR)
        os.system("make ARCH=arm " + \
                  "CROSS_COMPILE=arm-none-linux-gnueabi- O=" + \
                  LINUX_KERNEL_BUILDDIR)

    @staticmethod
    def clean(self):
        if os.path.exists(LINUX_KERNEL_BUILDDIR):
            shutil.rmtree(LINUX_KERNEL_BUILDDIR)

if __name__ == "__main__":
    container = Container()
    container.id = 0
    if len(sys.argv) == 1:
        LinuxBuilder.build_linux(container)
    elif "clean" == sys.argv[1]:
        LinuxBuilder.clean()
    else:
        print " Usage: %s [clean]" % (sys.argv[0])
