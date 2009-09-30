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

def build_linux_container(projpaths, container):
    linux_builder = LinuxBuilder(projpaths, container)
    linux_builder.build_linux()
    rootfs_builder = RootfsBuilder(projpaths, container)
    rootfs_builder.build_rootfs()
    linux_container_packer = LinuxContainerPacker(container, \
                                                  linux_builder, \
                                                  rootfs_builder)
    return linux_container_packer.pack_container()

def glob_by_walk(arg, dirname, names):
    ext, imglist = arg
    files = glob.glob(join(dirname, ext))
    imglist.extend(files)

def source_to_builddir(srcdir, id):
    cont_builddir = \
        os.path.relpath(srcdir, \
                        PROJROOT).replace("conts", \
                                          "cont" + str(id))
    return join(BUILDDIR, cont_builddir)

# We simply use SCons to figure all this out from container.id
# This is very similar to default container builder:
# In fact this notion may become a standard convention for
# calling specific bare containers
def build_posix_container(projpaths, container):
    images = []
    cwd = os.getcwd()
    os.chdir(POSIXDIR)
    print POSIXDIR
    print '\nBuilding the Posix Container...'
    scons_cmd = 'scons ' + 'cont=' + str(container.id)
    print "Issuing scons command: %s" % scons_cmd
    os.system(scons_cmd)
    builddir = source_to_builddir(POSIXDIR, container.id)
    os.path.walk(builddir, glob_by_walk, ['*.elf', images])
    container_packer = DefaultContainerPacker(container, images)
    return container_packer.pack_container()

# This simply calls SCons on a given container, and collects
# all images with .elf extension, instead of using whole classes
# for building and packing.
def build_default_container(projpaths, container):
    images = []
    cwd = os.getcwd()
    projdir = join(join(PROJROOT, 'conts'), container.name)
    os.chdir(projdir)
    os.system("scons")
    os.path.walk(projdir, glob_by_walk, ['*.elf', images])
    container_packer = DefaultContainerPacker(container, images)
    return container_packer.pack_container()


def build_all_containers():
    config = configuration_retrieve()

    cont_images = []
    for container in config.containers:
        if container.type == 'linux':
            pass
            cont_images.append(build_linux_container(projpaths, container))
        elif container.type == 'bare':
            cont_images.append(build_default_container(projpaths, container))
        elif container.type == 'posix':
            cont_images.append(build_posix_container(projpaths, container))
        else:
            print "Error: Don't know how to build " + \
                  "container of type: %s" % (container.type)
            exit(1)

    all_cont_packer = AllContainerPacker(cont_images, config.containers)

    return all_cont_packer.pack_all()

if __name__ == "__main__":
    build_all_containers()

