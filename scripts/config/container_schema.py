#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0
#
# Container Manifest Schema Validation
#
# Validates YAML container manifests against the schema and enforces
# constraints for memory layout, capabilities, and boot configuration.
#

import os
import sys
from typing import Any, Dict, List, Tuple


class ConfigError(Exception):
    """Rich error with YAML path context for debugging."""

    def __init__(self, message: str, yaml_path: str, value: Any = None):
        self.yaml_path = yaml_path
        self.value = value
        error_msg = f"{yaml_path}: {message}"
        if value is not None:
            error_msg += f" (got: {value})"
        super().__init__(error_msg)


# Schema version supported by this validator
SUPPORTED_SCHEMA_VERSION = 1

# Valid container types (linux removed - no Linux container support)
VALID_CONTAINER_TYPES = ["baremetal", "posix"]

# Valid capability targets
VALID_CAP_TARGETS = [
    "current_container",
    "current_pager_space",
]

# Valid baremetal projects
VALID_BAREMETAL_PROJECTS = [
    "empty",
    "hello_world",
    "threads_demo",
    "test_suite",
    "uart_service",
    "timer_service",
    "kmi_service",
    "mutex_demo",
    "ipc_demo",
]

# Constraints
MIN_PHYS_START = 0x40000  # Physical regions must start after kernel
MAX_CONTAINERS = 4
MAX_PHYS_REGIONS = 4
MAX_VIRT_REGIONS = 6
PAGE_SIZE = 0x1000


def validate_hex_or_int(value: Any, path: str) -> int:
    """Validate and convert hex string or int to integer."""
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        try:
            if value.startswith("0x") or value.startswith("0X"):
                return int(value, 16)
            return int(value)
        except ValueError:
            raise ConfigError("Invalid numeric value", path, value)
    raise ConfigError("Expected integer or hex string", path, value)


def validate_schema_version(manifest: Dict, path: str = "") -> None:
    """Validate schema version."""
    if "schemaVersion" not in manifest:
        raise ConfigError("Missing required field 'schemaVersion'", path or "root")

    version = manifest["schemaVersion"]
    if version != SUPPORTED_SCHEMA_VERSION:
        raise ConfigError(
            f"Unsupported schema version (supported: {SUPPORTED_SCHEMA_VERSION})",
            f"{path}schemaVersion" if path else "schemaVersion",
            version,
        )


def validate_container_type(container: Dict, idx: int) -> str:
    """Validate container type."""
    path = f"containers[{idx}].type"
    if "type" not in container:
        raise ConfigError("Missing required field 'type'", path)

    ctype = container["type"]
    if ctype not in VALID_CONTAINER_TYPES:
        raise ConfigError(
            f"Invalid container type (valid: {', '.join(VALID_CONTAINER_TYPES)})",
            path,
            ctype,
        )
    return ctype


def validate_memory_region(
    region: Dict, region_type: str, region_idx: int, container_idx: int
) -> Tuple[int, int]:
    """Validate a memory region and return (start, end)."""
    path_prefix = f"containers[{container_idx}].memory.{region_type}[{region_idx}]"

    if "start" not in region:
        raise ConfigError("Missing required field 'start'", f"{path_prefix}.start")
    if "end" not in region:
        raise ConfigError("Missing required field 'end'", f"{path_prefix}.end")

    start = validate_hex_or_int(region["start"], f"{path_prefix}.start")
    end = validate_hex_or_int(region["end"], f"{path_prefix}.end")

    # Validate start < end
    if end <= start:
        raise ConfigError(
            "Region end must exceed start",
            f"{path_prefix}",
            f"start=0x{start:x}, end=0x{end:x}",
        )

    # Validate alignment
    align = region.get("align", PAGE_SIZE)
    if isinstance(align, str):
        align = validate_hex_or_int(align, f"{path_prefix}.align")

    if start % align != 0:
        raise ConfigError(
            f"Start address must be aligned to 0x{align:x}",
            f"{path_prefix}.start",
            f"0x{start:x}",
        )

    # Physical region specific checks
    if region_type == "physical":
        if start < MIN_PHYS_START:
            raise ConfigError(
                f"Physical regions must start after 0x{MIN_PHYS_START:x} (kernel area)",
                f"{path_prefix}.start",
                f"0x{start:x}",
            )

    return start, end


def validate_memory(container: Dict, idx: int) -> None:
    """Validate container memory configuration."""
    path = f"containers[{idx}].memory"

    if "memory" not in container:
        raise ConfigError("Missing required field 'memory'", path)

    memory = container["memory"]

    # Validate physical regions
    if "physical" not in memory:
        raise ConfigError("Missing required field 'physical'", f"{path}.physical")

    phys_regions = memory["physical"]
    if not isinstance(phys_regions, list) or len(phys_regions) == 0:
        raise ConfigError("At least one physical region required", f"{path}.physical")

    if len(phys_regions) > MAX_PHYS_REGIONS:
        raise ConfigError(
            f"Maximum {MAX_PHYS_REGIONS} physical regions allowed",
            f"{path}.physical",
            len(phys_regions),
        )

    # Validate each physical region
    phys_ranges = []
    for i, region in enumerate(phys_regions):
        start, end = validate_memory_region(region, "physical", i, idx)
        phys_ranges.append((start, end))

    # Check for overlapping physical regions
    phys_ranges.sort()
    for i in range(1, len(phys_ranges)):
        if phys_ranges[i][0] < phys_ranges[i - 1][1]:
            raise ConfigError(
                "Physical memory regions must not overlap",
                f"{path}.physical",
                f"regions {i-1} and {i} overlap",
            )

    # Validate virtual regions
    if "virtual" not in memory:
        raise ConfigError("Missing required field 'virtual'", f"{path}.virtual")

    virt_regions = memory["virtual"]
    if not isinstance(virt_regions, list) or len(virt_regions) == 0:
        raise ConfigError("At least one virtual region required", f"{path}.virtual")

    if len(virt_regions) > MAX_VIRT_REGIONS:
        raise ConfigError(
            f"Maximum {MAX_VIRT_REGIONS} virtual regions allowed",
            f"{path}.virtual",
            len(virt_regions),
        )

    # Validate each virtual region
    for i, region in enumerate(virt_regions):
        validate_memory_region(region, "virtual", i, idx)


def validate_pager(container: Dict, idx: int) -> None:
    """Validate pager configuration."""
    path = f"containers[{idx}].pager"

    if "pager" not in container:
        raise ConfigError("Missing required field 'pager'", path)

    pager = container["pager"]

    if "lma" not in pager:
        raise ConfigError(
            "Missing required field 'lma' (load memory address)", f"{path}.lma"
        )
    if "vma" not in pager:
        raise ConfigError(
            "Missing required field 'vma' (virtual memory address)", f"{path}.vma"
        )

    lma = validate_hex_or_int(pager["lma"], f"{path}.lma")
    vma = validate_hex_or_int(pager["vma"], f"{path}.vma")

    # Validate LMA is in physical range
    memory = container.get("memory", {})
    phys_regions = memory.get("physical", [])

    lma_valid = False
    for region in phys_regions:
        start = validate_hex_or_int(region["start"], "")
        end = validate_hex_or_int(region["end"], "")
        if start <= lma < end:
            lma_valid = True
            break

    if not lma_valid and phys_regions:
        raise ConfigError(
            "Pager LMA must be within a physical memory region",
            f"{path}.lma",
            f"0x{lma:x}",
        )

    # Validate VMA is in virtual range
    virt_regions = memory.get("virtual", [])

    vma_valid = False
    for region in virt_regions:
        start = validate_hex_or_int(region["start"], "")
        end = validate_hex_or_int(region["end"], "")
        if start <= vma < end:
            vma_valid = True
            break

    if not vma_valid and virt_regions:
        raise ConfigError(
            "Pager VMA must be within a virtual memory region",
            f"{path}.vma",
            f"0x{vma:x}",
        )


def validate_capability_pool(pool: Dict, pool_name: str, container_idx: int) -> None:
    """Validate a capability pool configuration."""
    path = f"containers[{container_idx}].capabilities.pools.{pool_name}"

    if "enabled" not in pool:
        raise ConfigError("Missing required field 'enabled'", f"{path}.enabled")

    if pool["enabled"]:
        if "size" not in pool:
            raise ConfigError("Pool size required when enabled", f"{path}.size")
        size = pool["size"]
        if not isinstance(size, int) or size <= 0:
            raise ConfigError(
                "Pool size must be a positive integer", f"{path}.size", size
            )


def validate_capabilities(container: Dict, idx: int) -> None:
    """Validate container capabilities configuration."""
    path = f"containers[{idx}].capabilities"

    if "capabilities" not in container:
        raise ConfigError("Missing required field 'capabilities'", path)

    caps = container["capabilities"]

    # Validate pools
    if "pools" not in caps:
        raise ConfigError("Missing required field 'pools'", f"{path}.pools")

    pools = caps["pools"]
    required_pools = ["thread", "space", "mutex", "map", "cap"]

    for pool_name in required_pools:
        if pool_name not in pools:
            raise ConfigError(
                f"Missing required pool '{pool_name}'", f"{path}.pools.{pool_name}"
            )
        validate_capability_pool(pools[pool_name], pool_name, idx)


def validate_baremetal_project(container: Dict, idx: int) -> None:
    """Validate baremetal project selection."""
    path = f"containers[{idx}].project"

    if "project" not in container:
        raise ConfigError(
            "Missing required field 'project' for baremetal container", path
        )

    project = container["project"]
    if project not in VALID_BAREMETAL_PROJECTS:
        raise ConfigError(
            f"Invalid baremetal project (valid: {', '.join(VALID_BAREMETAL_PROJECTS)})",
            path,
            project,
        )


def validate_pager_region(
    region: Dict, region_name: str, container_idx: int
) -> Tuple[int, int]:
    """Validate a pager region (shm, task, or utcb) and return (start, end)."""
    path_prefix = f"containers[{container_idx}].pager_regions.{region_name}"

    if "start" not in region:
        raise ConfigError("Missing required field 'start'", f"{path_prefix}.start")
    if "end" not in region:
        raise ConfigError("Missing required field 'end'", f"{path_prefix}.end")

    start = validate_hex_or_int(region["start"], f"{path_prefix}.start")
    end = validate_hex_or_int(region["end"], f"{path_prefix}.end")

    if end <= start:
        raise ConfigError(
            "Region end must exceed start",
            f"{path_prefix}",
            f"start=0x{start:x}, end=0x{end:x}",
        )

    return start, end


def validate_pager_regions(container: Dict, idx: int) -> None:
    """Validate POSIX pager regions (shm, task, utcb)."""
    path = f"containers[{idx}].pager_regions"

    if "pager_regions" not in container:
        raise ConfigError(
            "Missing required field 'pager_regions' for POSIX container", path
        )

    pager_regions = container["pager_regions"]

    # Required regions for POSIX containers
    required_regions = ["shm", "task", "utcb"]

    for region_name in required_regions:
        if region_name not in pager_regions:
            raise ConfigError(
                f"Missing required pager region '{region_name}'",
                f"{path}.{region_name}",
            )
        validate_pager_region(pager_regions[region_name], region_name, idx)


def validate_container(container: Dict, idx: int) -> None:
    """Validate a single container configuration."""
    path = f"containers[{idx}]"

    # Validate id
    if "id" not in container:
        raise ConfigError("Missing required field 'id'", f"{path}.id")

    cid = container["id"]
    if cid != idx:
        raise ConfigError(
            f"Container id must match array index (expected {idx})", f"{path}.id", cid
        )

    # Validate name
    if "name" not in container:
        raise ConfigError("Missing required field 'name'", f"{path}.name")

    # Validate type
    ctype = validate_container_type(container, idx)

    # Type-specific validation
    if ctype == "baremetal":
        validate_baremetal_project(container, idx)
    elif ctype == "posix":
        validate_pager_regions(container, idx)

    # Validate memory
    validate_memory(container, idx)

    # Validate pager
    validate_pager(container, idx)

    # Validate capabilities
    validate_capabilities(container, idx)


def validate_manifest(manifest: Dict) -> None:
    """
    Validate a complete container manifest.

    Raises ConfigError on validation failure with detailed path context.
    """
    # Validate schema version
    validate_schema_version(manifest)

    # Validate containers array
    if "containers" not in manifest:
        raise ConfigError("Missing required field 'containers'", "containers")

    containers = manifest["containers"]
    if not isinstance(containers, list):
        raise ConfigError("'containers' must be an array", "containers")

    if len(containers) == 0:
        raise ConfigError("At least one container required", "containers")

    if len(containers) > MAX_CONTAINERS:
        raise ConfigError(
            f"Maximum {MAX_CONTAINERS} containers allowed",
            "containers",
            len(containers),
        )

    # Validate containers are sorted by id
    for i, container in enumerate(containers):
        if "id" in container and container["id"] != i:
            raise ConfigError(
                "Containers must be sorted by id",
                f"containers[{i}].id",
                f"expected {i}, got {container.get('id')}",
            )

    # Validate each container
    for idx, container in enumerate(containers):
        validate_container(container, idx)


def load_and_validate(yaml_path: str) -> Dict:
    """
    Load and validate a YAML manifest file.

    Returns the validated manifest dict.
    Raises ConfigError on validation failure.
    """
    # Use built-in yaml_parser (no PyYAML dependency)
    try:
        from . import yaml_parser
    except ImportError:
        import yaml_parser

    if not os.path.exists(yaml_path):
        raise ConfigError(f"File not found: {yaml_path}", "file")

    try:
        manifest = yaml_parser.load(yaml_path)
    except yaml_parser.YAMLParseError as e:
        raise ConfigError(f"YAML parse error: {e}", "file")
    except Exception as e:
        raise ConfigError(f"Failed to parse YAML: {e}", "file")

    if manifest is None:
        raise ConfigError("Empty manifest file", "file")

    validate_manifest(manifest)
    return manifest


def main():
    """CLI entry point for schema validation."""
    import argparse

    parser = argparse.ArgumentParser(
        description="Validate Codezero container manifest YAML files"
    )
    parser.add_argument("manifest", help="Path to YAML manifest file")
    parser.add_argument(
        "-q", "--quiet", action="store_true", help="Suppress output on success"
    )

    args = parser.parse_args()

    try:
        manifest = load_and_validate(args.manifest)
        if not args.quiet:
            n_containers = len(manifest["containers"])
            print(f"Manifest valid: {n_containers} container(s)")
            for c in manifest["containers"]:
                print(f"  [{c['id']}] {c['name']} ({c['type']})")
        sys.exit(0)
    except ConfigError as e:
        print(f"Validation error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
