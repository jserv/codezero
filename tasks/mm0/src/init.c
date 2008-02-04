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
#include <task.h>
#include <shm.h>

void init_utcb(void)
{
	struct task_ids ids;
	void *utcb_page = alloc_page(1); /* Allocate a utcb page */

	l4_getid(&ids);
	l4_map(utcb_page, __L4_ARM_Utcb(), 1, MAP_USR_RW_FLAGS, ids.tid);
}

void init_mm(struct initdata *initdata)
{
	/* Initialise the page and bank descriptors */
	init_physmem(initdata, membank);
	printf("%s: Initialised physmem.\n", __TASKNAME__);

	/* Initialise the page allocator on first bank. */
	init_page_allocator(membank[0].free, membank[0].end);
	printf("%s: Initialised page allocator.\n", __TASKNAME__);

	/* Initialise the zero page */
	init_zero_page();
	printf("%s: Initialised zero page.\n", __TASKNAME__);

	init_utcb();
	printf("%s: Initialised own utcb.\n", __TASKNAME__);

	/* Initialise the pager's memory allocator */
	kmalloc_init();
	printf("%s: Initialised kmalloc.\n", __TASKNAME__);

	shm_init();
	printf("%s: Initialised shm structures.\n", __TASKNAME__);

	/* Give the kernel some memory to use for its allocators */
	l4_kmem_grant(__pfn(alloc_page(__pfn(SZ_1MB))), __pfn(SZ_1MB));
}

/* Create temporary run-time files in memory to test with mmap */
void init_boot_files(struct initdata *initdata)
{
	struct vm_file *f;
	struct svc_image *img;
	struct bootdesc *bd = initdata->bootdesc;

	INIT_LIST_HEAD(&initdata->boot_file_list);
	for (int i = BOOTDESC_IMAGE_START; i < bd->total_images; i++) {
		img = &bd->images[i];
		if (!(!strcmp(img->name, "fs0") || !strcmp(img->name, "test0")))
			continue; /* Img is not what we want */

		f = kzalloc(sizeof(*f));
		INIT_LIST_HEAD(&f->list);
		INIT_LIST_HEAD(&f->page_cache_list);
		list_add(&f->list, &initdata->boot_file_list);

		/*
		 * For boot files, we use the physical address of the memory
		 * file as its inode.
		 */
		f->inode.i_addr = img->phys_start;
		f->length = img->phys_end - img->phys_start;
		f->pager = &default_file_pager;
	}
}

void initialise(void)
{
	request_initdata(&initdata);

	init_mm(&initdata);

	init_boot_files(&initdata);
	// printf("INITTASK: Initialised mock-up bootfiles.\n");

	init_pm(&initdata);
	// printf("INITTASK: Initialised the memory/process manager.\n");
}

