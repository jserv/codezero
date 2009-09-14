#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
import os, sys, shelve, shutil, re
from projpaths import *

class Container:
    def __init__(self):
        self.name = None
        self.type = None
        self.id = None
        self.lma_start = None
        self.lma_end = None
        self.vma_start = None
        self.vma_end = None

class configuration:

    def __init__(self):
        self.arch = None
        self.subarch = None
        self.platform = None
        self.all = []
        self.containers = []

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

    # TODO: Carry this over to Container() as static method???
    def get_container_parameter(self, id, param, val):
        if param[:len("VIRT_START")] == "VIRT_START":
            self.containers[id].vma_start = val
        elif param[:len("VIRT_END")] == "VIRT_END":
            self.containers[id].vma_end = val
        elif param[:len("PHYS_START")] == "PHYS_START":
            self.containers[id].lma_start = val
        elif param[:len("PHYS_END")] == "PHYS_END":
            self.containers[id].lma_end = val
        else:
            param1, param2 = param.split("_", 2)
            if param1 == "TYPE":
                if param2 == "LINUX":
                    self.containers[id].type = "linux"
                elif param2 == "C0_POSIX":
                    self.containers[id].type = "cps"
                elif param2 == "BARE":
                    self.containers[id].type = "bare"

    # Extract parameters for containers
    def get_container_parameters(self, name, val):
        matchobj = re.match(r"(CONFIG_CONT){1}([0-9]){1}(\w+)", name)
        if not matchobj:
            return None

        prefix, idstr, param = matchobj.groups()
        id = int(idstr)

        # Create and add new container if this id was not seen
        self.check_add_container(id)

        # Get rid of '_' in front
        param = param[1:]

        # Check and store info on this parameter
        self.get_container_parameter(id, param, val)

    def check_add_container(self, id):
        for cont in self.containers:
            if id == cont.id:
                return

        # New container created. TODO: Pass id to constructor
        container = Container()
        container.id = id
        self.containers.append(container)

        # Make sure elements in order for indexed accessing
        self.containers.sort()

def configuration_save(config):
    if not os.path.exists(CONFIG_SHELVE_DIR):
        os.mkdir(CONFIG_SHELVE_DIR)

    config_shelve = shelve.open(CONFIG_SHELVE)
    config_shelve["configuration"] = config

    config_shelve["arch"] = config.arch
    config_shelve["subarch"] = config.subarch
    config_shelve["platform"] = config.platform
    config_shelve["all_symbols"] = config.all
    config_shelve.close()


def configuration_retrieve():
    # Get configuration information
    config_shelve = shelve.open(CONFIG_SHELVE)
    config = config_shelve["configuration"]
    return config
