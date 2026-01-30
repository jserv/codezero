#! /usr/bin/env python3
# -*- mode: python; coding: utf-8; -*-
import glob
import os
import re
import shelve
from functools import cmp_to_key

from .caps import prepare_capability, create_default_capabilities
from .lib import conv_hex
from .projpaths import CONFIG_SHELVE, CONFIG_SHELVE_DIR


class CapabilityList:
    def __init__(self):
        self.physmem = {}
        self.physmem["START"] = {}
        self.physmem["END"] = {}
        self.virtmem = {}
        self.virtmem["START"] = {}
        self.virtmem["END"] = {}
        self.caps = {}
        self.virt_regions = 0
        self.phys_regions = 0


class Container:
    def __init__(self, id):
        self.dirname = None
        self.duplicate = 0
        self.name = None
        self.type = None
        self.id = id
        self.pager_lma = 0
        self.pager_vma = 0
        self.pager_size = 0
        self.pager_rw_pheader_start = 0
        self.pager_rw_pheader_end = 0
        self.pager_rx_pheader_start = 0
        self.pager_rx_pheader_end = 0
        self.pager_task_region_start = 0
        self.pager_task_region_end = 0
        self.pager_shm_region_start = 0
        self.pager_shm_region_end = 0
        self.pager_utcb_region_start = 0
        self.pager_utcb_region_end = 0
        self.linux_zreladdr = 0
        self.linux_page_offset = 0
        self.linux_phys_offset = 0
        self.linux_rootfs_address = 0
        self.caplist = {}
        self.caplist["PAGER"] = CapabilityList()
        self.caplist["CONTAINER"] = CapabilityList()

    def print_self(self):
        print("\nContainer %d" % self.id)
        print("------------")
        print("Container type:                      %s" % self.type)
        print("Container Name:                      %s" % self.name)
        print("Container Pager lma:                 %s" % conv_hex(self.pager_lma))
        print("Container Pager vma:                 %s" % conv_hex(self.pager_vma))
        print(
            "Container Pager shm region start:    %s"
            % conv_hex(self.pager_shm_region_start)
        )
        print(
            "Container Pager shm region end:      %s"
            % conv_hex(self.pager_shm_region_end)
        )
        print(
            "Container Pager task region start:   %s"
            % conv_hex(self.pager_task_region_start)
        )
        print(
            "Container Pager task region end:     %s"
            % conv_hex(self.pager_task_region_end)
        )
        print(
            "Container Pager utcb region start:   %s"
            % conv_hex(self.pager_utcb_region_start)
        )
        print(
            "Container Pager utcb region end:     %s"
            % conv_hex(self.pager_utcb_region_end)
        )
        print("Container Virtual regions:           %s" % self.caps.virt_regions)
        print("Container Physical regions:          %s" % self.caps.phys_regions)
        print("Container Pager Virtual regions:     %s" % self.pager_caps.virt_regions)
        print("Container Pager Physical regions:    %s" % self.pager_caps.phys_regions)
        # print('Container Capabilities:             %s' % self.caps)
        print("\n")


class configuration:

    def __init__(self):
        # Mapping between cpu and gcc flags for it.
        # Optimized solution to derive gcc arch flag from cpu
        # gcc flag here is "-march"
        #                          cpu          -march flag
        self.arch_to_gcc_flag = (
            ["ARM926", "armv5te"],
            ["ARM1136", "armv6"],
            ["ARM11MPCORE", "armv6k"],
            ["CORTEXA8", "armv7-a"],
            ["CORTEXA9", "armv7-a"],
        )
        self.arch = None
        self.subarch = None
        self.platform = None
        self.cpu = None
        self.gcc_arch_flag = None
        self.toolchain_userspace = None
        self.toolchain_kernel = None
        self.all = []
        self.smp = False
        self.ncpu = 0
        self.containers = []
        self.ncontainers = 0

    # Get all name value symbols
    def get_all(self, name, val):
        self.all.append([name, val])

    # Convert line to name value pair, if possible
    def line_to_name_value(self, line):
        parts = line.split()
        if len(parts) >= 3:
            if parts[0] == "#define":
                return parts[1], parts[2]
        return None

    def get_ncpu(self, name, value):
        """Check if SMP enabled and get NCPU count."""
        if name.startswith("CONFIG_SMP"):
            self.smp = bool(value)
        elif name.startswith("CONFIG_NCPU"):
            self.ncpu = int(value)

    def get_arch(self, name, val):
        """Extract architecture from a name value pair."""
        if name.startswith("CONFIG_ARCH_"):
            self.arch = name.split("_", 3)[2].lower()

    def get_subarch(self, name, val):
        """Extract subarch from a name value pair."""
        if name.startswith("CONFIG_SUBARCH_"):
            self.subarch = name.split("_", 3)[2].lower()

    def get_platform(self, name, val):
        """Extract platform from a name value pair."""
        if name.startswith("CONFIG_PLATFORM_"):
            self.platform = name.split("_", 3)[2].lower()

    def get_cpu(self, name, val):
        """Extract cpu from a name value pair."""
        if name.startswith("CONFIG_CPU_"):
            cpu_type = name.split("_", 3)[2]
            self.cpu = cpu_type.lower()
            # Derive gcc "-march" flag
            for cputype, archflag in self.arch_to_gcc_flag:
                if cputype == cpu_type:
                    self.gcc_arch_flag = archflag
                    break

    def get_toolchain(self, name, val):
        """Extract toolchain paths from name value pair."""
        if name.startswith("CONFIG_TOOLCHAIN_USERSPACE"):
            self.toolchain_userspace = val.split('"', 2)[1]
        elif name.startswith("CONFIG_TOOLCHAIN_KERNEL"):
            self.toolchain_kernel = val.split('"', 2)[1]

    def get_ncontainers(self, name, val):
        """Extract number of containers."""
        if name.startswith("CONFIG_CONTAINERS"):
            self.ncontainers = int(val)

    # Memory region pattern: [PAGER_]VIRT|PHYS{n}_START|END
    _MEM_REGION_PATTERN = re.compile(
        r"(PAGER_)?(VIRT|PHYS)(\d)_(START|END)"
    )

    def get_container_parameter(self, id, param, val):
        """Parse and store a container configuration parameter."""
        cont = self.containers[id]

        # Simple prefix-based parameters
        simple_params = {
            "PAGER_LMA": ("pager_lma", int, 0),
            "PAGER_VMA": ("pager_vma", int, 0),
            "PAGER_UTCB_START": ("pager_utcb_region_start", int, 0),
            "PAGER_UTCB_END": ("pager_utcb_region_end", int, 0),
            "PAGER_SHM_START": ("pager_shm_region_start", int, 0),
            "PAGER_SHM_END": ("pager_shm_region_end", int, 0),
            "PAGER_TASK_START": ("pager_task_region_start", int, 0),
            "PAGER_TASK_END": ("pager_task_region_end", int, 0),
            "LINUX_ZRELADDR": ("linux_zreladdr", int, 0),
        }

        for prefix, (attr, conv, base) in simple_params.items():
            if param.startswith(prefix):
                setattr(cont, attr, conv(val, base))
                return

        # Linux offset params with side effects
        if param.startswith("LINUX_PAGE_OFFSET"):
            offset = int(val, 0)
            cont.linux_page_offset = offset
            cont.pager_vma += offset
            return
        if param.startswith("LINUX_PHYS_OFFSET"):
            offset = int(val, 0)
            cont.linux_phys_offset = offset
            cont.pager_lma += offset
            return
        if param.startswith("LINUX_ROOTFS_ADDRESS"):
            cont.linux_rootfs_address += int(val, 0)
            return

        # Memory region parameters
        match = self._MEM_REGION_PATTERN.match(param)
        if match:
            pager, memtype, regionid, startend = match.groups()
            regionid = int(regionid)
            owner = "PAGER" if pager else "CONTAINER"
            caplist = cont.caplist[owner]

            if memtype == "VIRT":
                caplist.virtmem[startend][regionid] = val
                caplist.virt_regions = max(caplist.virt_regions, regionid + 1)
            else:
                caplist.physmem[startend][regionid] = val
                caplist.phys_regions = max(caplist.phys_regions, regionid + 1)
            return

        # Name and project parameters
        if param.startswith("OPT_NAME"):
            cont.name = val[1:-1].lower()
            return
        if param.startswith("BAREMETAL_PROJ_"):
            cont.dirname = param.split("_", 2)[2].lower()
            return

        # Capability parameters
        if param.startswith("PAGER_CAP_"):
            prepare_capability(cont, "PAGER", param.split("_", 2)[2], val)
            return
        if param.startswith("CAP_"):
            prepare_capability(cont, "CONTAINER", param.split("_", 1)[1], val)
            return

        # Container type
        if param.startswith("TYPE_"):
            if val == "1" or val == 1:
                type_name = param.split("_", 1)[1]
                cont.type = type_name.lower()

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
        # self.containers_print(self.containers)

    # Used for sorting container members,
    # with this we are sure containers are sorted by id value
    @staticmethod
    def compare_containers(cont, cont2):
        if cont.id < cont2.id:
            return -1
        if cont.id == cont2.id:
            print("compare_containers: Error, containers have same id.")
            exit(1)
        if cont.id > cont2.id:
            return 1

    def check_add_container(self, id):
        for cont in self.containers:
            if id == cont.id:
                return

        # If symbol to describe number of containers
        # Has already been visited, use that number
        # as an extra checking.
        if self.ncontainers > 0:
            # Sometimes unwanted symbols slip through
            if id >= self.ncontainers:
                return

        container = Container(id)
        self.containers.append(container)

        # Make sure elements in order for indexed accessing
        self.containers.sort(key=cmp_to_key(self.compare_containers))

    def config_print(self):
        print("\nConfiguration")
        print("-------------")
        print("Arch:        %s, %s" % (self.arch, self.subarch))
        print("Platform:    %s" % self.platform)
        # print('Symbols:    %s' % self.all)
        print("Containers:  %d" % self.ncontainers)
        self.containers_print()

    def containers_print(self):
        for c in self.containers:
            c.print_self()


def configuration_save(config):
    if not os.path.exists(CONFIG_SHELVE_DIR):
        os.mkdir(CONFIG_SHELVE_DIR)

    config_shelve = shelve.open(CONFIG_SHELVE)
    config_shelve["configuration"] = config

    config_shelve["arch"] = config.arch
    config_shelve["subarch"] = config.subarch
    config_shelve["platform"] = config.platform
    config_shelve["cpu"] = config.cpu
    config_shelve["all_symbols"] = config.all
    config_shelve.close()


def configuration_retrieve():
    """Get configuration from shelve storage.

    Shelve creates files with various extensions (.db, .dir, .bak)
    depending on the underlying database module.
    """
    if not glob.glob(CONFIG_SHELVE + "*"):
        return None
    try:
        with shelve.open(CONFIG_SHELVE) as config_shelve:
            return config_shelve["configuration"]
    except Exception:
        return None
