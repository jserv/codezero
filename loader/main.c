#include <elf/elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "arch.h"

/* These symbols are defined by the linker script. */
extern char _start_kernel[];
extern char _end_kernel[];

/* This is a kernel symbol exported to loader's linker script from kernel build */
extern char bkpt_phys_to_virt[];

void load_container_images(unsigned long start, unsigned long end)
{
	struct elf_image *elf_img = (struct elf_image *)start;
	int nsect_headers;

	// Find all section headers
	
}

int main(void)
{
	void *kernel_entry = 0;

	arch_init();

	printf("ELF Loader: Started.\n");

	printf("Loading the kernel...\n");
	load_elf_image(&kernel_entry, _start_kernel, _end_kernel);

	printf("Loading containers...\n");
	load_container_images(_start_containers, _end_containers)

	printf("elf-loader:\tkernel entry point is %p\n", kernel_entry);
//	arch_start_kernel(kernel_entry);

	printf("elf-loader:\tKernel start failed!\n");

	return -1;
}

