
#include <elf/elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "arch.h"

/* These symbols are defined by the linker scripts. */
extern char _start_kernel[];
extern char _end_kernel[];
extern char _start_containers[];
extern char _end_containers[];

/* This is a kernel symbol exported to loader's linker script from kernel build */
extern char bkpt_phys_to_virt[];

int
main(void)
{
	void *kernel_entry = NULL;

	arch_init();

	printf("elf-loader:\tStarted\n");

	printf("Loading the kernel...\n");
//	load_image(&kernel_entry, _start_kernel, _end_kernel);

	printf("elf-loader:\tkernel entry point is %p\n", kernel_entry);
//	arch_start_kernel(kernel_entry);

	printf("elf-loader:\tKernel start failed!\n");

	return -1;
}

