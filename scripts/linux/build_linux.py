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

# Create linux kernel build directory path as:
# conts/linux -> build/cont[0-9]/linux
def source_to_builddir(srcdir, id):
    cont_builddir = \
        os.path.relpath(srcdir, \
                        PROJROOT).replace("conts", \
                                          "cont" + str(id))
    return join(BUILDDIR, cont_builddir)

class LinuxBuilder:
    LINUX_KERNEL_BUILDDIR = None
    LINUX_KERNELDIR = None

    def __init__(self, pathdict, container):
        self.LINUX_KERNELDIR = pathdict["LINUX_KERNELDIR"]

        # Calculate linux kernel build directory
        self.LINUX_KERNEL_BUILDDIR = \
            source_to_builddir(LINUX_KERNELDIR, container.id)

    def build_linux(self):
        print '\nBuilding the linux kernel...'
        os.chdir(self.LINUX_KERNELDIR)
        if not os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            os.makedirs(self.LINUX_KERNEL_BUILDDIR)
        os.system("make defconfig ARCH=arm O=" + self.LINUX_KERNEL_BUILDDIR)
        os.system("make ARCH=arm " + \
                  "CROSS_COMPILE=arm-none-linux-gnueabi- O=" + \
                  self.LINUX_KERNEL_BUILDDIR)
        print 'Done...'

    def clean(self):
        print 'Cleaning linux kernel build...'
        if os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            shutil.rmtree(self.LINUX_KERNEL_BUILDDIR)
        print 'Done...'

if __name__ == "__main__":
    # This is only a default test case
    container = Container()
    container.id = 0
    linux_builder = LinuxBuilder(projpaths, container)

    if len(sys.argv) == 1:
        linux_builder.build_linux()
    elif "clean" == sys.argv[1]:
        linux_builder.clean()
    else:
        print " Usage: %s [clean]" % (sys.argv[0])
