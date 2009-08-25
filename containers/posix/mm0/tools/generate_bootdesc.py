#!/usr/bin/python
import os
import sys

compiler_prefix = "arm-none-linux-gnueabi-"
objdump = "objdump"
command = "-t"
image_name = "inittask.axf"
linkoutput_file_suffix = "-linkinfo.txt"
linkoutput_file = image_name + linkoutput_file_suffix

def generate_bootdesc():
	command = compiler_prefix + objdump + " -t " + image_name + " > " + linkoutput_file
	print command
	os.system(command)
	f = open(linkoutput_file, "r")

	while True:
		line = f.readline()
		if len(line) is 0:
			break
		if "_start" in line or "_end" in line:
			print line
	f.close()

if __name__ == "__main__":
	generate_bootdesc()

