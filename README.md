# Codezero Microkernel

## What is Codezero?

Codezero is an L4 microkernel that has been written from scratch. It targets
embedded systems and its purpose is to act as a secure embedded hypervisor. It
aims to become the most modern L4 implementation by evolving the L4 microkernel
API into the future. In a nutshell, Codezero provides all the basic mechanism
to abstract away the hardware, build OS services, isolate applications and
fine-grained security in a single package.


## Why the name Codezero?

The project philosophy is to create the simplest and leanest microkernel that
is generic and applicable to many different applications. Feature creep is what
we don't have in Codezero. Simple, elegant design is the project philosophy.


## Building

### Prerequisites

- [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`, `arm-none-eabi-ld`, etc.)
- Python 3
- [SCons build system](https://scons.org)
- QEMU ARM emulator (for running)

### Build Steps

1. Set up the toolchain path:
```shell
export PATH=/path/to/arm-none-eabi/bin:$PATH
```

2. Build with default configuration (`hello_world` demo):
```shell
./build.py
```

3. Build with a specific configuration:
```shell
./build.py -d <defconfig_name>
```

Available configurations:
- `pb926` - Default PB926 with `hello_world`
- `test_suite` - Kernel test suite
- `ipc_demo` - IPC demonstration
- `threads_demo` - Threading demonstration
- `kmi_service` - Keyboard/mouse input service
- `posix_test0` - POSIX compatibility test

4. Build options:
```shell
./build.py -C       # Interactive configuration (menuconfig)
./build.py -j4      # Parallel build with 4 jobs
./build.py -c       # Clean build artifacts
./build.py -h       # Show all options
```

## Running

Run the built image in QEMU:
```shell
./tools/run-qemu
```

For debugging with GDB:
```shell
./tools/run-qemu-gdb
```

This starts QEMU with the ARM926 CPU emulating a VersatilePB board, with GDB
server enabled for debugging.


## What is the license?

The current release is distributed under GNU General Public License Version 3.

The third party source code under the directories `loader/`, `tools/`,
`loader/libs/c`, and `loader/libs/elf` have their own copyright and licenses,
separate from this project. All third party source code is open source in the
OSI definition. Please check these directories for their respective licenses.
