#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
import os, sys, shelve, shutil
from projpaths import *

class container:
    name = None
    type = None
    id = None
    lma_start = None
    lma_end = None
    vma_start = None
    vma_end = None

class configuration:
    arch = None
    subarch = None
    platform = None
    all = []
    containers = []
    # Get all name value symbols
    def get_all(self, name, val):
        self.all.append([name, val])

    # Convert line to name value pair, if possible
    def line_to_name_value(self, line):
        parts = line.split()
        if len(parts) > 0:
            if parts[0] == "#define":
                return parts[1], parts[2]
        return None

    # Extract architecture from a name value pair
    def get_arch(self, name, val):
        if name[:len("CONFIG_ARCH_")] == "CONFIG_ARCH_":
            parts = name.split("_", 3)
            self.arch = parts[2].lower()

    # Extract subarch from a name value pair
    def get_subarch(self, name, val):
        if name[:len("CONFIG_SUBARCH_")] == "CONFIG_SUBARCH_":
            parts = name.split("_", 3)
            self.subarch = parts[2].lower()

    # Extract platform from a name value pair
    def get_platform(self, name, val):
        if name[:len("CONFIG_PLATFORM_")] == "CONFIG_PLATFORM_":
            parts = name.split("_", 3)
            self.platform = parts[2].lower()

    # Extract number of containers
    def get_ncontainers(self, name, val):
        if name[:len("CONFIG_CONTAINERS")] == "CONFIG_CONTAINERS":
            self.ncontainers = val

def configuration_retrieve():
    # Get configuration information
    config_shelve = shelve.open(CONFIG_SHELVE)
    configuration = config_shelve["configuration"]
    return configuration
