/*
 * Initialise the system.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <mm/alloc_page.h>
#include <kmalloc/kmalloc.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/utcb.h>
#include <task.h>
#include <shm.h>
#include <file.h>
#include <init.h>
#include <utcb.h>


void init_mm(struct initdata *initdata)
{
	/* Initialise the page and bank descriptors */
	init_physmem(initdata, membank);
	printf("%s: Initialised physmem.\n", __TASKNAME__);

	/* Initialise the page allocator on first bank. */
	init_page_allocator(membank[0].free, membank[0].end);
	printf("%s: Initialised page allocator.\n", __TASKNAME__);

	/* Initialise the pager's memory allocator */
	kmalloc_init();
	printf("%s: Initialised kmalloc.\n", __TASKNAME__);

	/* Initialise the zero page */
	init_devzero();
	printf("%s: Initialised devzero.\n", __TASKNAME__);

	/* Initialise in-memory boot files */
	init_boot_files(initdata);
	printf("%s: Initialised in-memory boot files.\n", __TASKNAME__);

	shm_init();
	printf("%s: Initialised shm structures.\n", __TASKNAME__);

	if (utcb_pool_init() < 0) {
		printf("UTCB initialisation failed.\n");
		BUG();
	}
	printf("%s: Initialised utcb address pool.\n", __TASKNAME__);

	/* Give the kernel some memory to use for its allocators */
	l4_kmem_grant(__pfn(alloc_page(__pfn(SZ_1MB))), __pfn(SZ_1MB));
}

void initialise(void)
{
	request_initdata(&initdata);

	init_mm(&initdata);

	init_pm(&initdata);
	printf("%s: Initialised the memory/process manager.\n", __TASKNAME__);
}

