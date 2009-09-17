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
from scripts.linux.build_linux import *
from scripts.linux.build_rootfs import *
from pack import *
from packall import *

def build_container(container):
    if container.type == "linux":
        linux_builder = LinuxBuilder(projpaths, container)
        linux_builder.build_linux()
        rootfs_builder = RootfsBuilder(projpaths, container)
        rootfs_builder.build_rootfs()
        linux_container_packer = LinuxContainerPacker(container, \
                                                      linux_builder, \
                                                      rootfs_builder)
        return linux_container_packer.pack_container()

    else:
        print "Error: Don't know how to build " + \
              "container of type: %s" % (container.type)
        Exit(1)

def main():
    container_images = []

    config = configuration_retrieve()
    for container in config.containers:
        container_images.append(build_container(container))

    all_cont_packer = AllContainerPacker(container_images, config.containers)

    all_cont_packer.pack_all()

if __name__ == "__main__":
    main()

