#!/bin/bash

#Export symbols from kernel to loader's linker script
#Debuggers can use these symbols as a stop point for loading ksymtab before mmu is on.
./tools/ksym_to_lds.py

cp build/start.axf loader/
cp tasks/mm0/mm0.axf loader/
cp tasks/fs0/fs0.axf loader/
cp tasks/test0/test0.axf loader/
cp tasks/bootdesc/bootdesc.axf loader/
cd loader
scons -c
scons
cp final.axf ../build

if [ -n "$SAMBA_DIR" ]
then
  cp final.axf $SAMBA_DIR
  cp start.axf $SAMBA_DIR
fi
cd ../build
arm-none-eabi-objdump -d start.axf > start.txt
