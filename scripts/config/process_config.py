#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0
#
# Configuration Processor
#
# Merges Kconfig output with YAML container manifests to generate
# the final configuration header (include/l4/config.h).
#
# This script is the bridge between:
#   - Kconfig (kernel feature selection)
#   - YAML manifests (container system description)
#
# Output:
#   - include/l4/config.h: Complete configuration header
#   - build/configdata/: Python shelve for build system integration
#

import argparse
import os
import re
import sys
from typing import Any, Dict

# Project paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

# Add project to path for imports
sys.path.insert(0, PROJECT_ROOT)
sys.path.insert(0, SCRIPT_DIR)

# Use built-in yaml_parser (no PyYAML dependency)
try:
    from .yaml_parser import load as yaml_load, YAMLParseError
except ImportError:
    from yaml_parser import load as yaml_load, YAMLParseError


class ConfigError(Exception):
    """Configuration processing error."""

    pass


def parse_kconfig_header(header_path: str) -> Dict[str, Any]:
    """
    Parse a Kconfig-generated header file into a dict of symbols.

    Returns dict mapping CONFIG_* names to values.
    """
    symbols = {}

    if not os.path.exists(header_path):
        raise ConfigError(f"Kconfig header not found: {header_path}")

    with open(header_path, "r") as f:
        for line in f:
            line = line.strip()

            # Match #define CONFIG_FOO value
            match = re.match(r"#define\s+(CONFIG_\w+)\s+(.+)", line)
            if match:
                name = match.group(1)
                value = match.group(2)

                # Parse value type
                if value.startswith('"') and value.endswith('"'):
                    # String value
                    symbols[name] = value[1:-1]
                elif value.isdigit():
                    # Integer
                    symbols[name] = int(value)
                elif value.startswith("0x"):
                    # Hex integer
                    symbols[name] = int(value, 16)
                else:
                    # Boolean or other
                    symbols[name] = value

    return symbols


def load_yaml_manifest(manifest_path: str) -> Dict:
    """Load and parse a YAML container manifest."""
    if not os.path.exists(manifest_path):
        raise ConfigError(f"Manifest not found: {manifest_path}")

    try:
        return yaml_load(manifest_path)
    except YAMLParseError as e:
        raise ConfigError(f"YAML parse error: {e}")


def hex_value(value: Any) -> int:
    """Convert value to integer, handling hex strings."""
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        if value.startswith("0x") or value.startswith("0X"):
            return int(value, 16)
        return int(value)
    raise ConfigError(f"Cannot convert to integer: {value}")


def generate_container_symbols(manifest: Dict) -> Dict[str, Any]:
    """
    Generate CONFIG_* symbols from container manifest.

    These symbols match the format expected by the existing kernel code
    (compatible with CML2 output).
    """
    symbols = {}
    containers = manifest.get("containers", [])

    symbols["CONFIG_CONTAINERS"] = len(containers)

    for container in containers:
        idx = container["id"]
        prefix = f"CONFIG_CONT{idx}_"

        # Container type
        ctype = container["type"].upper()
        symbols[f"{prefix}TYPE_{ctype}"] = 1

        # Mark other types as not set
        for t in ["BAREMETAL", "POSIX"]:
            if t != ctype:
                symbols[f"{prefix}TYPE_{t}"] = 0

        # Container name
        symbols[f"{prefix}OPT_NAME"] = container["name"]

        # Baremetal project selection
        if container["type"] == "baremetal":
            project = container.get("project", "empty")
            project_upper = project.upper()
            symbols[f"{prefix}BAREMETAL_PROJ_{project_upper}"] = 1

        # Pager addresses
        pager = container.get("pager", {})
        if "lma" in pager:
            symbols[f"{prefix}PAGER_LMA"] = hex_value(pager["lma"])
        if "vma" in pager:
            symbols[f"{prefix}PAGER_VMA"] = hex_value(pager["vma"])

        # Derived pager symbols (for cinfo.c compatibility)
        symbols[f"{prefix}PAGER_LOAD_ADDR"] = symbols.get(f"{prefix}PAGER_LMA", 0)
        symbols[f"{prefix}PAGER_VIRT_ADDR"] = symbols.get(f"{prefix}PAGER_VMA", 0)
        symbols[f"{prefix}START_PC_ADDR"] = symbols.get(f"{prefix}PAGER_VMA", 0)

        # Physical memory regions
        # Baremetal: pager IS the container, so only PAGER-level symbols to avoid
        #            duplicate physmem capabilities (which cause double-unmap errors)
        # Non-baremetal: generate both container-level and pager-level symbols
        memory = container.get("memory", {})
        phys_regions = memory.get("physical", [])
        is_baremetal = ctype == "BAREMETAL"

        # Container-level region count: 0 for baremetal (no container-level caps)
        symbols[f"{prefix}PHYSMEM_REGIONS"] = 0 if is_baremetal else len(phys_regions)

        for i, region in enumerate(phys_regions):
            # Container-level symbols only for non-baremetal types
            if not is_baremetal:
                symbols[f"{prefix}PHYS{i}_START"] = hex_value(region["start"])
                symbols[f"{prefix}PHYS{i}_END"] = hex_value(region["end"])
            # Pager-level physical memory (always generated for all types)
            symbols[f"{prefix}PAGER_PHYS{i}_START"] = hex_value(region["start"])
            symbols[f"{prefix}PAGER_PHYS{i}_END"] = hex_value(region["end"])

        # Virtual memory regions
        # Same logic as physical: baremetal skips container-level to avoid duplicates
        virt_regions = memory.get("virtual", [])

        # Container-level region count: 0 for baremetal (no container-level caps)
        symbols[f"{prefix}VIRTMEM_REGIONS"] = 0 if is_baremetal else len(virt_regions)

        for i, region in enumerate(virt_regions):
            # Container-level symbols only for non-baremetal types
            if not is_baremetal:
                symbols[f"{prefix}VIRT{i}_START"] = hex_value(region["start"])
                symbols[f"{prefix}VIRT{i}_END"] = hex_value(region["end"])
            # Pager-level virtual memory (always generated for all types)
            symbols[f"{prefix}PAGER_VIRT{i}_START"] = hex_value(region["start"])
            symbols[f"{prefix}PAGER_VIRT{i}_END"] = hex_value(region["end"])

        # POSIX-specific pager regions (mm0 needs these for managing memory)
        pager_regions = container.get("pager_regions", {})

        # Shared memory region
        shm_region = pager_regions.get("shm", {})
        if shm_region:
            symbols[f"{prefix}PAGER_SHM_START"] = hex_value(shm_region["start"])
            symbols[f"{prefix}PAGER_SHM_END"] = hex_value(shm_region["end"])

        # Task address space region
        task_region = pager_regions.get("task", {})
        if task_region:
            symbols[f"{prefix}PAGER_TASK_START"] = hex_value(task_region["start"])
            symbols[f"{prefix}PAGER_TASK_END"] = hex_value(task_region["end"])

        # UTCB mappings region
        utcb_region = pager_regions.get("utcb", {})
        if utcb_region:
            symbols[f"{prefix}PAGER_UTCB_START"] = hex_value(utcb_region["start"])
            symbols[f"{prefix}PAGER_UTCB_END"] = hex_value(utcb_region["end"])

        # Pager-level capabilities (PAGER_CAP_* prefix)
        # These capabilities are managed by the pager and go to pager caplist
        caps = container.get("capabilities", {})
        pools = caps.get("pools", {})

        pool_mapping = {
            "thread": "THREADPOOL",
            "space": "SPACEPOOL",
            "mutex": "MUTEXPOOL",
            "map": "MAPPOOL",
            "cap": "CAPPOOL",
        }

        for pool_key, pool_name in pool_mapping.items():
            pool = pools.get(pool_key, {})
            enabled = pool.get("enabled", False)
            symbols[f"{prefix}PAGER_CAP_{pool_name}_USE"] = 1 if enabled else 0
            if enabled:
                symbols[f"{prefix}PAGER_CAP_{pool_name}_SIZE"] = pool.get("size", 0)

        # Capability controls (pager-level)
        cap_controls = ["tctrl", "exregs", "ipc", "capctrl", "umutex"]
        for cap_name in cap_controls:
            cap = caps.get(cap_name, {})
            cap_upper = cap_name.upper()
            enabled = cap.get("enabled", False)
            symbols[f"{prefix}PAGER_CAP_{cap_upper}_USE"] = 1 if enabled else 0

            if enabled:
                target = cap.get("target", "current_container")
                if target == "current_container":
                    symbols[
                        f"{prefix}PAGER_CAP_{cap_upper}_TARGET_CURRENT_CONTAINER"
                    ] = 1
                    symbols[
                        f"{prefix}PAGER_CAP_{cap_upper}_TARGET_CURRENT_PAGER_SPACE"
                    ] = 0
                elif target == "current_pager_space":
                    symbols[
                        f"{prefix}PAGER_CAP_{cap_upper}_TARGET_CURRENT_CONTAINER"
                    ] = 0
                    symbols[
                        f"{prefix}PAGER_CAP_{cap_upper}_TARGET_CURRENT_PAGER_SPACE"
                    ] = 1

        # IRQ control (pager-level)
        irqctrl = caps.get("irqctrl", {})
        symbols[f"{prefix}PAGER_CAP_IRQCTRL_USE"] = (
            1 if irqctrl.get("enabled", False) else 0
        )

        # Custom capabilities (default to disabled)
        for i in range(4):
            symbols[f"{prefix}PAGER_CAP_CUSTOM{i}_USE"] = 0

        # Device capabilities
        devices = container.get("devices", [])
        device_types = ["UART1", "UART2", "UART3", "TIMER1"]
        for dev_type in device_types:
            symbols[f"{prefix}PAGER_CAP_DEVICE_{dev_type}_USE"] = 0

        for device in devices:
            dev_name = device.get("name", "").upper()
            if dev_name in device_types:
                symbols[f"{prefix}PAGER_CAP_DEVICE_{dev_name}_USE"] = 1

    return symbols


def generate_config_header(
    kconfig_symbols: Dict[str, Any], container_symbols: Dict[str, Any], output_path: str
) -> None:
    """
    Generate the final config.h header file.

    Merges Kconfig symbols with container-derived symbols.
    """
    # Merge symbols (container symbols take precedence for conflicts)
    all_symbols = {}
    all_symbols.update(kconfig_symbols)
    all_symbols.update(container_symbols)

    # Determine architecture for __ARCH__ macros
    arch = "arm" if all_symbols.get("CONFIG_ARCH_ARM") else "unknown"

    if all_symbols.get("CONFIG_SUBARCH_V5"):
        subarch = "v5"
    elif all_symbols.get("CONFIG_SUBARCH_V6"):
        subarch = "v6"
    else:
        subarch = "unknown"

    if all_symbols.get("CONFIG_CPU_ARM926"):
        cpu = "arm926"
    elif all_symbols.get("CONFIG_CPU_ARM1136"):
        cpu = "arm1136"
    elif all_symbols.get("CONFIG_CPU_ARM11MPCORE"):
        cpu = "arm11mpcore"
    else:
        cpu = "unknown"

    if all_symbols.get("CONFIG_PLATFORM_PB926"):
        platform = "pb926"
    elif all_symbols.get("CONFIG_PLATFORM_EB"):
        platform = "eb"
    else:
        platform = "unknown"

    # Create output directory
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with open(output_path, "w") as f:
        f.write("/*\n")
        f.write(" * Codezero Configuration Header\n")
        f.write(" * Auto-generated - DO NOT EDIT\n")
        f.write(" *\n")
        f.write(" * Generated by scripts/config/process_config.py\n")
        f.write(" */\n\n")
        f.write("#ifndef __L4_CONFIG_H__\n")
        f.write("#define __L4_CONFIG_H__\n\n")

        # Write architecture macros (used by include paths)
        f.write("/* Architecture identification macros */\n")
        f.write(f"#define __ARCH__ {arch}\n")
        f.write(f"#define __SUBARCH__ {subarch}\n")
        f.write(f"#define __PLATFORM__ {platform}\n")
        f.write(f"#define __CPU__ {cpu}\n\n")

        # Write Kconfig symbols first
        f.write("/* Kernel configuration (from Kconfig) */\n")
        kconfig_keys = sorted(
            [k for k in all_symbols.keys() if not k.startswith("CONFIG_CONT")]
        )
        for key in kconfig_keys:
            if key.startswith("CONFIG_"):
                value = all_symbols[key]
                if isinstance(value, str):
                    f.write(f'#define {key} "{value}"\n')
                elif isinstance(value, int):
                    if value >= 0x10000:
                        f.write(f"#define {key} 0x{value:x}\n")
                    else:
                        f.write(f"#define {key} {value}\n")
                elif value == 1 or value is True:
                    f.write(f"#define {key} 1\n")

        # Write container symbols
        f.write("\n/* Container configuration (from YAML manifest) */\n")
        container_keys = sorted(
            [k for k in all_symbols.keys() if k.startswith("CONFIG_CONT")]
        )

        # Group by container
        current_cont = None
        for key in container_keys:
            # Extract container number
            match = re.match(r"CONFIG_CONT(\d+)_", key)
            if match:
                cont_num = match.group(1)
                if cont_num != current_cont:
                    f.write(f"\n/* Container {cont_num} */\n")
                    current_cont = cont_num

            value = all_symbols[key]
            if isinstance(value, str):
                f.write(f'#define {key} "{value}"\n')
            elif isinstance(value, int):
                if value >= 0x10000:
                    f.write(f"#define {key} 0x{value:x}\n")
                else:
                    f.write(f"#define {key} {value}\n")

        f.write("\n#endif /* __L4_CONFIG_H__ */\n")

    print(f"Configuration header written to {output_path}")


def process_config(kconfig_header: str, manifest_path: str, output_path: str) -> None:
    """
    Main configuration processing function.

    Args:
        kconfig_header: Path to Kconfig-generated header (build/kconfig.h)
        manifest_path: Path to YAML container manifest
        output_path: Path for output config.h
    """
    # Parse Kconfig header
    print(f"Loading Kconfig symbols from {kconfig_header}")
    kconfig_symbols = parse_kconfig_header(kconfig_header)
    print(f"  Found {len(kconfig_symbols)} Kconfig symbols")

    # Load and validate YAML manifest
    print(f"Loading container manifest from {manifest_path}")

    # Import validation
    from container_schema import load_and_validate

    manifest = load_and_validate(manifest_path)

    # Generate container symbols
    print("Generating container configuration symbols")
    container_symbols = generate_container_symbols(manifest)
    print(f"  Generated {len(container_symbols)} container symbols")

    # Generate final header
    generate_config_header(kconfig_symbols, container_symbols, output_path)


def main():
    parser = argparse.ArgumentParser(
        description="Process Kconfig + YAML to generate config.h"
    )
    parser.add_argument(
        "-k",
        "--kconfig",
        default=os.path.join(PROJECT_ROOT, "build", "kconfig.h"),
        help="Path to Kconfig-generated header",
    )
    parser.add_argument(
        "-m", "--manifest", required=True, help="Path to YAML container manifest"
    )
    parser.add_argument(
        "-o",
        "--output",
        default=os.path.join(PROJECT_ROOT, "include", "l4", "config.h"),
        help="Output path for config.h",
    )

    args = parser.parse_args()

    try:
        process_config(args.kconfig, args.manifest, args.output)
    except ConfigError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
        import traceback

        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
