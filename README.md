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


## What is the license?

The current release is distributed under GNU General Public License Version 3.

The third party source code under the directories loader/ tools/ libs/c
libs/elf have their own copyright and licenses, separate from this project. All
third party source code is open source in the OSI definition. Please check
these directories for their respective licenses.
