#include <elf/elf.h>
#include <elf/elf32.h>
#include <stdio.h>
#include <stdlib.h>
#include "arch.h"

/* These symbols are defined by the linker script. */
extern char _start_kernel[];
extern char _end_kernel[];
extern char _start_containers[];
extern char _end_containers[];

/* This is a kernel symbol exported to loader's linker script from kernel build */
extern char bkpt_phys_to_virt[];

/*
void load_container_images(unsigned long start, unsigned long end)
{
	struct elf_image *elf_img = (struct elf_image *)start;
	int nsect_headers;

	// Find all section headers

}
*/

int load_elf_image(unsigned long **entry, void *filebuf)
{
	if (!elf32_checkFile((struct Elf32_Header *)filebuf)) {
		**entry = (unsigned long)elf32_getEntryPoint((struct Elf32_Header *)filebuf);
		printf("Entry point: %lx\n", **entry);
	} else {
		printf("Not a valid elf image.\n");
		return -1;
	}
	if (!elf_loadFile(filebuf, 1)) {
		printf("Elf image seems valid, but unable to load.\n");
		return -1;
	}
	return 0;
}

int main(void)
{
	unsigned long *kernel_entry;

	arch_init();

	printf("ELF Loader: Started.\n");

	printf("Loading the kernel...\n");
	load_elf_image(&kernel_entry, (void *)_start_kernel);

	printf("Loading containers...\n");
//	load_container_images(_start_containers, _end_containers)

	printf("elf-loader:\tkernel entry point is %lx\n", *kernel_entry);
//	arch_start_kernel(kernel_entry);

//	printf("elf-loader:\tKernel start failed!\n");

	return -1;
}

