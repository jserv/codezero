#! /usr/bin/env python3
# -*- mode: python; coding: utf-8; -*-
#
# Configuration system using Kconfig + YAML manifests.
#
# Replaces the legacy CML2 configuration system with:
# 1. Kconfig for kernel feature selection
# 2. YAML manifests for container system description
#
import os
import sys
from optparse import OptionParser
from os.path import join

from .projpaths import (
    PROJROOT, KCONFIG_DIR, BUILDDIR, CONFIGS_DIR, CONFIG_H,
    CONFIG_SHELVE_DIR, KCONFIG_DOT_CONFIG, KCONFIG_HEADER,
    KCONFIG_DEFCONFIG_DIR, define_config_dependent_projpaths
)
from .configuration import (
    configuration, configuration_save, create_default_capabilities
)

sys.path.append(PROJROOT)
sys.path.append(KCONFIG_DIR)
from scripts.baremetal.baremetal_generator import BaremetalContGenerator
from scripts.kernel.generate_kernel_cinfo import *

# Import Kconfig library
try:
    from kconfiglib import Kconfig
    KCONFIG_AVAILABLE = True
except ImportError:
    KCONFIG_AVAILABLE = False

# Import menuconfig for interactive configuration
try:
    from menuconfig import menuconfig
    MENUCONFIG_AVAILABLE = True
except ImportError:
    MENUCONFIG_AVAILABLE = False


def config_header_to_symbols(header_path, config):
    """Parse config.h and populate configuration object."""
    with open(header_path) as header_file:
        for line in header_file:
            pair = config.line_to_name_value(line)
            if pair is not None:
                name, value = pair
                config.get_all(name, value)
                config.get_cpu(name, value)
                config.get_arch(name, value)
                config.get_subarch(name, value)
                config.get_platform(name, value)
                config.get_ncpu(name, value)
                config.get_ncontainers(name, value)
                config.get_container_parameters(name, value)
                config.get_toolchain(name, value)


def add_default_caps(config):
    """Add default capabilities (tctrl, exregs) for each container."""
    for c in config.containers:
        create_default_capabilities(c)


def detect_manifest_from_kconfig(kconf):
    """Auto-detect manifest file based on Kconfig container selection."""
    # Map Kconfig symbols to (project_dir, manifest_suffix, container_type)
    # Container type is "baremetal" or "posix"
    container_map = {
        "CONT_BAREMETAL_EMPTY": ("empty", "empty", "baremetal"),
        "CONT_BAREMETAL_HELLO_WORLD": ("hello_world", "helloworld", "baremetal"),
        "CONT_BAREMETAL_IPC_DEMO": ("ipc_demo", "ipc_demo", "baremetal"),
        "CONT_BAREMETAL_KMI_SERVICE": ("kmi_service", "kmi_service", "baremetal"),
        "CONT_BAREMETAL_MUTEX_DEMO": ("mutex_demo", "mutex_demo", "baremetal"),
        "CONT_BAREMETAL_TEST_SUITE": ("test_suite", "test_suite", "baremetal"),
        "CONT_BAREMETAL_THREADS_DEMO": ("threads_demo", "threads_demo", "baremetal"),
        "CONT_BAREMETAL_TIMER_SERVICE": ("timer_service", "timer_service", "baremetal"),
        "CONT_BAREMETAL_UART_SERVICE": ("uart_service", "uart_service", "baremetal"),
        "CONT_POSIX_MM0": ("mm0", "posix_mm0", "posix"),
        "CONT_POSIX_TEST0": ("test0", "posix_test0", "posix"),
    }

    # Find first enabled container
    selected_project = None
    selected_manifest = None
    selected_type = "baremetal"
    for sym_name, (project, manifest, ctype) in container_map.items():
        sym = kconf.syms.get(sym_name)
        if sym and sym.str_value == "y":
            selected_project = project
            selected_manifest = manifest
            selected_type = ctype
            break

    if not selected_manifest:
        selected_project = "hello_world"
        selected_manifest = "helloworld"
        selected_type = "baremetal"

    # Determine platform prefix from Kconfig
    platform = "pb926"  # Default
    if (
        kconf.syms.get("PLATFORM_PB926")
        and kconf.syms["PLATFORM_PB926"].str_value == "y"
    ):
        platform = "pb926"
    elif kconf.syms.get("PLATFORM_EB") and kconf.syms["PLATFORM_EB"].str_value == "y":
        platform = "eb"
    elif (
        kconf.syms.get("PLATFORM_PBA9") and kconf.syms["PLATFORM_PBA9"].str_value == "y"
    ):
        platform = "pba9"

    manifest_name = f"{platform}_{selected_manifest}.yaml"
    manifest_path = join(CONFIGS_DIR, manifest_name)

    # Fallback to pb926 if platform-specific not found
    if not os.path.exists(manifest_path):
        fallback_path = join(CONFIGS_DIR, f"pb926_{selected_manifest}.yaml")
        if os.path.exists(fallback_path):
            manifest_path = fallback_path
        else:
            # Auto-generate manifest for the selected container
            manifest_path = generate_manifest_for_container(
                platform, selected_project, selected_manifest, selected_type
            )

    return manifest_path


def generate_manifest_for_container(platform, project, manifest_suffix, container_type):
    """
    Auto-generate a YAML manifest file for a container.

    Creates a standard manifest with default memory layout and capabilities.
    Supports both baremetal and posix container types.
    """
    manifest_name = f"{platform}_{manifest_suffix}.yaml"
    manifest_path = join(CONFIGS_DIR, manifest_name)

    # Container name (project name + "0" suffix)
    container_name = f"{project}0"

    if container_type == "posix":
        manifest_content = _generate_posix_manifest(platform, project, container_name)
    else:
        manifest_content = _generate_baremetal_manifest(
            platform, project, container_name
        )

    # Ensure configs directory exists
    os.makedirs(CONFIGS_DIR, exist_ok=True)

    # Write manifest file
    with open(manifest_path, "w") as f:
        f.write(manifest_content)

    print(f"Auto-generated manifest: {manifest_path}")
    return manifest_path


def _generate_baremetal_manifest(platform, project, container_name):
    """Generate manifest content for baremetal containers."""
    return f"""# Codezero Container Manifest
# {platform.upper()} {project.replace('_', ' ').title()} Configuration
#
# Auto-generated manifest for {project} baremetal container.
# Modify as needed for specific requirements.

schemaVersion: 1

# Reference to kernel Kconfig defconfig
kconfig: {platform}

containers:
  - id: 0
    name: "{container_name}"
    type: baremetal
    project: {project}

    # Pager configuration
    pager:
      lma: 0x100000          # Physical load address
      vma: 0xa0000000        # Virtual memory address

    # Physical memory regions
    memory:
      physical:
        - start: 0x100000
          end: 0xe00000

      # Virtual memory regions
      virtual:
        - start: 0xa0000000
          end: 0xb0000000

    # Capability pools
    capabilities:
      pools:
        thread:
          enabled: true
          size: 64
        space:
          enabled: true
          size: 64
        mutex:
          enabled: true
          size: 100
        map:
          enabled: true
          size: 800
        cap:
          enabled: true
          size: 32

      # Thread control capability
      tctrl:
        enabled: true
        target: current_container

      # Exchange registers capability
      exregs:
        enabled: true
        target: current_container

      # IPC capability
      ipc:
        enabled: true
        target: current_container

      # Capability control
      capctrl:
        enabled: true
        target: current_container

      # Userspace mutex
      umutex:
        enabled: true
        target: current_container

      # IRQ control
      irqctrl:
        enabled: true

    # Device capabilities (add device mappings here if needed)
    devices: []
"""


def _generate_posix_manifest(platform, project, container_name):
    """Generate manifest content for POSIX containers."""
    return f"""# Codezero Container Manifest
# {platform.upper()} {project.replace('_', ' ').title()} POSIX Configuration
#
# Auto-generated manifest for {project} POSIX container.
# Modify as needed for specific requirements.

schemaVersion: 1

# Reference to kernel Kconfig defconfig
kconfig: {platform}

containers:
  - id: 0
    name: "{container_name}"
    type: posix

    # Pager configuration
    pager:
      lma: 0x100000          # Physical load address
      vma: 0xa0000000        # Virtual memory address

    # Physical memory regions
    memory:
      physical:
        - start: 0x100000
          end: 0xe00000

      # Virtual memory regions
      # POSIX containers need multiple virtual regions:
      # - Region 0: Pager code/data (0xa0000000-0xb0000000)
      # - Region 1: Shared memory (0x80000000-0x90000000)
      # - Region 2: Task space (0x40000000-0x80000000)
      # - Region 3: UTCB region (0xf8100000-0xf8200000)
      virtual:
        - start: 0xa0000000
          end: 0xb0000000
        - start: 0x80000000
          end: 0x90000000
        - start: 0x40000000
          end: 0x80000000
        - start: 0xf8100000
          end: 0xf8200000

    # POSIX pager regions (required for mm0)
    # These define where mm0 manages different memory types
    pager_regions:
      shm:
        start: 0x80000000
        end: 0x90000000
      task:
        start: 0x40000000
        end: 0x80000000
      utcb:
        start: 0xf8100000
        end: 0xf8200000

    # Capability pools
    capabilities:
      pools:
        thread:
          enabled: true
          size: 64
        space:
          enabled: true
          size: 64
        mutex:
          enabled: true
          size: 100
        map:
          enabled: true
          size: 800
        cap:
          enabled: true
          size: 32

      # Thread control capability
      tctrl:
        enabled: true
        target: current_container

      # Exchange registers capability
      exregs:
        enabled: true
        target: current_container

      # IPC capability
      ipc:
        enabled: true
        target: current_container

      # Capability control
      capctrl:
        enabled: true
        target: current_container

      # Userspace mutex
      umutex:
        enabled: true
        target: current_container

      # IRQ control
      irqctrl:
        enabled: true

    # Device capabilities (add device mappings here if needed)
    devices: []
"""


def build_parse_options():
    """Parse command line options."""
    usage = "\n\t%prog [options] arg\n\n\
             \r\t** :  Override all other options provided"
    parser = OptionParser(usage, version="codezero 4.0")

    parser.add_option(
        "-m",
        "--manifest",
        type="string",
        dest="manifest",
        help="Specify YAML container manifest file.",
    )

    parser.add_option(
        "-d",
        "--defconfig",
        type="string",
        dest="defconfig",
        help="Load a Kconfig defconfig before processing manifest.",
    )

    parser.add_option(
        "-C",
        "--configure",
        action="store_true",
        dest="config",
        help="Configure only (launches menuconfig). Use -b for batch mode.",
    )

    parser.add_option(
        "-b",
        "--batch",
        action="store_true",
        dest="batch",
        default=False,
        help="Batch mode (non-interactive).",
    )

    parser.add_option(
        "-j",
        "--jobs",
        type="str",
        dest="jobs",
        default="1",
        help="Enable parallel build with SCons.",
    )

    parser.add_option(
        "-p",
        "--print-config",
        action="store_true",
        default=False,
        dest="print_config",
        help="Print configuration**",
    )

    parser.add_option(
        "-c",
        "--clean",
        action="store_true",
        dest="clean",
        default=False,
        help="Do cleanup excluding configuration files**",
    )

    parser.add_option(
        "-x",
        "--clean-all",
        action="store_true",
        dest="clean_all",
        default=False,
        help="Do cleanup including configuration files**",
    )

    options, args = parser.parse_args()

    # Determine if configuration is needed
    options.config_only = 0
    if options.config:
        options.config_only = 1
    elif (
        options.manifest
        or options.defconfig
        or not os.path.exists(BUILDDIR)
        or not os.path.exists(CONFIG_SHELVE_DIR)
    ):
        options.config = 1

    return options, args


def configure_system(options, args):
    """
    Configure using Kconfig + YAML manifest.

    1. Kconfig for kernel feature selection
    2. YAML manifest for container system description
    """
    if not options.config:
        return

    if not KCONFIG_AVAILABLE:
        print("Error: kconfiglib not found. Install from tools/kconfig/")
        sys.exit(1)

    if not os.path.exists(BUILDDIR):
        os.mkdir(BUILDDIR)

    # Step 1: Load Kconfig defconfig
    kconf = Kconfig(join(PROJROOT, "Kconfig"), warn_to_stderr=False)

    if options.defconfig:
        defconfig_path = options.defconfig
        if not os.path.exists(defconfig_path):
            # Try in defconfig directory
            defconfig_path = join(KCONFIG_DEFCONFIG_DIR, options.defconfig)
            if not os.path.exists(defconfig_path):
                defconfig_path = join(
                    KCONFIG_DEFCONFIG_DIR, options.defconfig + "_defconfig"
                )

        if os.path.exists(defconfig_path):
            print(f"Loading defconfig from {defconfig_path}")
            kconf.load_config(defconfig_path)
        else:
            print(f"Warning: defconfig '{options.defconfig}' not found, using defaults")
    elif os.path.exists(KCONFIG_DOT_CONFIG):
        print(f"Loading existing config from {KCONFIG_DOT_CONFIG}")
        kconf.load_config(KCONFIG_DOT_CONFIG)
    else:
        # Use pb926 as default
        default_defconfig = join(KCONFIG_DEFCONFIG_DIR, "pb926_defconfig")
        if os.path.exists(default_defconfig):
            print(f"Loading default config from {default_defconfig}")
            kconf.load_config(default_defconfig)

    # Run interactive menuconfig if -C/--configure and not batch mode
    if options.config_only and not options.batch:
        if MENUCONFIG_AVAILABLE:
            print("Launching menuconfig...")
            # Set config file path for menuconfig to save to
            os.environ["KCONFIG_CONFIG"] = KCONFIG_DOT_CONFIG
            menuconfig(kconf)
            # Reload config after menuconfig (user may have changed values)
            if os.path.exists(KCONFIG_DOT_CONFIG):
                kconf.load_config(KCONFIG_DOT_CONFIG)
        else:
            print("Warning: menuconfig not available (curses not installed?)")
            print("Using non-interactive configuration")

    # Write .config and kconfig.h
    kconf.write_config(KCONFIG_DOT_CONFIG)
    kconf.write_autoconf(KCONFIG_HEADER)
    print(f"Kconfig written to {KCONFIG_DOT_CONFIG}")

    # Step 2: Process YAML manifest
    if not options.manifest:
        # Auto-detect manifest based on Kconfig container selection
        options.manifest = detect_manifest_from_kconfig(kconf)
        print(f"Auto-selected manifest: {options.manifest}")

    if not os.path.exists(options.manifest):
        print(f"Error: Manifest not found: {options.manifest}")
        sys.exit(1)

    # Step 3: Run process_config.py to merge Kconfig + YAML
    from .process_config import process_config
    from .container_schema import load_and_validate

    print(f"Processing manifest: {options.manifest}")

    # Validate manifest
    manifest = load_and_validate(options.manifest)
    print(f"Manifest valid: {len(manifest['containers'])} container(s)")

    # Generate final config.h
    if not os.path.exists(os.path.dirname(CONFIG_H)):
        os.mkdir(os.path.dirname(CONFIG_H))

    process_config(KCONFIG_HEADER, options.manifest, CONFIG_H)

    # Step 4: Create configuration object for build system
    config = configuration()

    # Parse the generated config.h
    config_header_to_symbols(CONFIG_H, config)
    add_default_caps(config)

    # Save configuration
    configuration_save(config)

    # Initialize config dependent projpaths
    define_config_dependent_projpaths(config)

    # Generate baremetal container files if new ones defined
    baremetal_cont_gen = BaremetalContGenerator()
    baremetal_cont_gen.baremetal_container_generate(config)

    print("Kconfig+YAML configuration complete")
    return config


def config_files_cleanup():
    """Remove configuration and build files."""
    os.system("rm -f " + CONFIG_H)
    os.system("rm -rf " + BUILDDIR)
    return None


if __name__ == "__main__":
    opts, args = build_parse_options()

    # We force configuration when calling this script
    opts.config = 1

    configure_system(opts, args)
