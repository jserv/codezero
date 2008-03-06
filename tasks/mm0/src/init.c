/*
 * Initialise the system.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <kdata.h>
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

/*
 * Initialise the utcb virtual address pool and its own utcb.
 * NOTE: This allocates memory so kmalloc must be initialised first.
 */
void init_utcb(void)
{
	void *utcb_virt, *utcb_page;

	/* Allocate and map one for self */
	if (utcb_pool_init() < 0)
		printf("UTCB initialisation failed.\n");
	utcb_virt = utcb_vaddr_new();
	printf("%s: Mapping 0x%x as utcb to self.\n", __TASKNAME__, utcb_virt);
	utcb_page = alloc_page(1);
	l4_map(utcb_page, utcb_virt, 1, MAP_USR_RW_FLAGS, self_tid());

	/* Also initialise the utcb reference that is inside l4lib. */
	utcb = utcb_virt;
}

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
	init_boot_files();
	printf("%s: Initialised in-memory boot files.\n", __TASKNAME__);

	shm_init();
	printf("%s: Initialised shm structures.\n", __TASKNAME__);

	init_utcb();
	printf("%s: Initialised own utcb.\n", __TASKNAME__);

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

