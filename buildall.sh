#!/bin/bash

rm -rf build
echo -e "\n--- Building kernel --- "
scons configure
scons build
cd tasks
echo -e "\n--- Building libraries --- "
cd libmem
scons
cd ../libl4
scons
cd ../libposix
scons
echo -e "\n--- Building tasks --- "
cd ../mm0
scons
cd ../fs0
scons
cd ../test0
scons
echo -e "\n--- Building bootdesc ---"
cd ../bootdesc
scons
cd ../..
echo -e "\n--- Packing all up ---"
./packer.sh
echo -e "\n--- Build Completed ---\n"
