#! /usr/bin/env python3
# -*- mode: python; coding: utf-8; -*-
import re
import sys
from os.path import join

from .projpaths import PROJROOT, CONFIG_H
from .lib import conv_hex


def get_conts_memory_regions(phys_virt, array_start, array_end):
    """
    Extract memory regions from config.h for overlap checking.

    Matches both container-level and pager-level regions:
    - Container-level: CONFIG_CONT*_PHYS*_START/END (non-baremetal containers)
    - Pager-level: CONFIG_CONT*_PAGER_PHYS*_START/END (all containers)

    For baremetal containers, only pager-level regions exist (pager IS the container).
    For non-baremetal containers, both levels exist but use the same ranges.
    Duplicates are filtered to avoid false overlap detection.
    """
    # Track regions as (start, end) pairs keyed by container and region index
    # Key format: "CONT{n}_{PHYS|VIRT}{m}" to deduplicate container vs pager level
    regions = {}

    # Patterns to extract container ID and region index
    # Matches: CONFIG_CONT0_PHYS0_START or CONFIG_CONT0_PAGER_PHYS0_START
    cont_pattern = r"CONFIG_CONT(\d+)_" + phys_virt + r"(\d+)_(START|END)"
    pager_pattern = r"CONFIG_CONT(\d+)_PAGER_" + phys_virt + r"(\d+)_(START|END)"

    with open(join(PROJROOT, CONFIG_H), "r") as file:
        for line in file:
            # Try pager-level pattern first (covers baremetal)
            match = re.search(pager_pattern, line)
            if match:
                cont_id, region_id, start_end = match.groups()
                key = f"CONT{cont_id}_{phys_virt}{region_id}"
                # Parse hex value from end of line
                begin = line.rfind(" ")
                try:
                    value = int(line[begin:].strip(), 16)
                except ValueError:
                    continue
                if key not in regions:
                    regions[key] = [None, None]
                if start_end == "START":
                    regions[key][0] = value
                else:
                    regions[key][1] = value
                continue

            # Try container-level pattern (for non-baremetal)
            match = re.search(cont_pattern, line)
            if match:
                cont_id, region_id, start_end = match.groups()
                key = f"CONT{cont_id}_{phys_virt}{region_id}"
                # Parse hex value from end of line
                begin = line.rfind(" ")
                try:
                    value = int(line[begin:].strip(), 16)
                except ValueError:
                    continue
                if key not in regions:
                    regions[key] = [None, None]
                if start_end == "START":
                    regions[key][0] = value
                else:
                    regions[key][1] = value

    # Extract paired start/end values, skipping incomplete regions
    for key, (start, end) in regions.items():
        if start is not None and end is not None:
            array_start.append(start)
            array_end.append(end)


def check_memory_overlap(phys_virt, array_start, array_end):
    """Check for overlapping memory regions using O(n^2) pairwise comparison."""
    n = len(array_start)
    for i in range(n):
        s1, e1 = array_start[i], array_end[i]
        for j in range(i + 1, n):
            s2, e2 = array_start[j], array_end[j]
            # Overlap if one region starts before the other ends
            if s1 < e2 and s2 < e1:
                print("Memory overlap between containers!!!")
                print(f"overlapping ranges: {conv_hex(s1)}-{conv_hex(e1)} "
                      f"and {conv_hex(s2)}-{conv_hex(e2)}")
                print()
                sys.exit(1)


def phys_region_sanity_check():
    phys_start = []
    phys_end = []
    get_conts_memory_regions("PHYS", phys_start, phys_end)
    check_memory_overlap("PHYS", phys_start, phys_end)


def virt_region_sanity_check():
    virt_start = []
    virt_end = []
    get_conts_memory_regions("VIRT", virt_start, virt_end)
    check_memory_overlap("VIRT", virt_start, virt_end)


def sanity_check_conts():
    phys_region_sanity_check()
    virt_region_sanity_check()
