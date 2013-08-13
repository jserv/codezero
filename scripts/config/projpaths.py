#! /usr/bin/env python2.7
# -*- mode: python; coding: utf-8; -*-

import os, sys, shelve, shutil
from os.path import join

# Way to get project root from any script importing this one :-)
PROJROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '../..'))

BUILDDIR        = join(PROJROOT, 'build')
TOOLSDIR        = join(PROJROOT, 'tools')
LOADERDIR       = join(PROJROOT, 'loader')
KERNEL_HEADERS  = join(PROJROOT, 'include')
SCRIPTS_DIR     = join(PROJROOT, 'scripts')
KERNEL_ELF      = join(BUILDDIR, 'kernel.elf')
FINAL_ELF       = join(BUILDDIR, 'final.elf')

USERLIBS_RELDIR = 'conts/userlibs'
USERLIBS_DIR    = join(PROJROOT, USERLIBS_RELDIR)

LIBL4_RELDIR    = join(USERLIBS_RELDIR, 'libl4')
LIBL4_DIR       = join(PROJROOT, LIBL4_RELDIR)
LIBL4_INCLUDE   = join(LIBL4_DIR, 'include')
LIBL4_LIBPATH   = join(BUILDDIR, LIBL4_RELDIR)

LIBC_RELDIR     = join(USERLIBS_RELDIR, 'libc')
LIBC_DIR        = join(PROJROOT, LIBC_RELDIR)
LIBC_LIBPATH    = join(BUILDDIR, LIBC_RELDIR)
LIBC_INCLUDE    = [join(LIBC_DIR, 'include')]

LIBDEV_RELDIR       = join(USERLIBS_RELDIR, 'libdev')
LIBDEV_DIR          = join(PROJROOT, LIBDEV_RELDIR)
LIBDEV_INCLUDE      = [join(LIBDEV_DIR, 'uart/include'), join(LIBDEV_DIR, 'include')]
LIBDEV_USER_LIBPATH = join(join(BUILDDIR, LIBDEV_RELDIR), 'sys-userspace')
LIBDEV_BAREMETAL_LIBPATH = join(join(BUILDDIR, LIBDEV_RELDIR), 'sys-baremetal')

LIBMEM_RELDIR   = join(USERLIBS_RELDIR, 'libmem')
LIBMEM_DIR      = join(PROJROOT, LIBMEM_RELDIR)
LIBMEM_LIBPATH  = join(BUILDDIR, LIBMEM_RELDIR)
LIBMEM_INCLUDE  = join(LIBMEM_DIR, 'include')

CML2_CONFIG_SRCDIR  = join(SCRIPTS_DIR, 'config/cml')
CML2_CONT_DEFFILE   = join(CML2_CONFIG_SRCDIR, 'container_ruleset.template')
CML2TOOLSDIR        = join(TOOLSDIR, 'cml2-tools')
CML2_COMPILED_RULES = join(BUILDDIR, 'rules.compiled')
CML2_CONFIG_FILE    = join(BUILDDIR, 'config.cml')
CML2_CONFIG_H       = join(BUILDDIR, 'config.h')
CML2_AUTOGEN_RULES  = join(BUILDDIR, 'config.rules')
CONFIG_H            = join(PROJROOT, 'include/l4/config.h')
CONFIG_SHELVE_DIR   = join(BUILDDIR, 'configdata')
CONFIG_SHELVE_FILENAME = 'configuration'
CONFIG_SHELVE       = join(CONFIG_SHELVE_DIR, CONFIG_SHELVE_FILENAME)
KERNEL_CINFO_PATH   = join(PROJROOT, "src/generic/cinfo.c")
LINUXDIR            = join(PROJROOT, 'conts/linux')
LINUX_KERNELDIR     = join(LINUXDIR, 'kernel-2.6.34')
LINUX_ROOTFSDIR     = join(LINUXDIR, 'rootfs')
LINUX_ATAGSDIR      = join(LINUXDIR, 'atags')

POSIXDIR            = join(PROJROOT, 'conts/posix')
POSIX_BOOTDESCDIR   = join(POSIXDIR, 'bootdesc')

projpaths = {
    'LINUX_ATAGSDIR'    : LINUX_ATAGSDIR,
    'LINUX_ROOTFSDIR'   : LINUX_ROOTFSDIR,
    'LINUX_KERNELDIR'   : LINUX_KERNELDIR,
    'LINUXDIR'          : LINUXDIR,
    'BUILDDIR'          : BUILDDIR,
    'POSIXDIR'          : POSIXDIR,
    'POSIX_BOOTDESCDIR' : POSIX_BOOTDESCDIR
}

def define_config_dependent_projpaths(config):
    LIBC_INCLUDE.append([join(LIBC_DIR, 'include/arch/' + config.arch)])
    return None

