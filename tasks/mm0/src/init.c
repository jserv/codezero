/*
 * Initialise the system.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
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
	struct bootdesc *bd = initdata->bootdesc;
	int total_files = bd->total_images;
	struct vm_file *memfile;
	struct svc_image *img;

	memfile = kzalloc(sizeof(struct vm_file) * total_files);
	initdata->memfile = memfile;
	BUG();
	for (int i = BOOTDESC_IMAGE_START; i < total_files; i++) {
		img = &bd->images[i];
		/*
		 * I have left the i_addr as physical on purpose. The inode is
		 * not a readily usable memory address, its simply a unique key
		 * that represents that file. Here, we use the physical address
		 * of the memory file as that key. The pager must take action in
		 * order to make use of it.
		 */
		memfile[i].inode.i_addr = img->phys_start;
		memfile[i].length = img->phys_end - img->phys_start;
		memfile[i].pager = &default_file_pager;
		INIT_LIST_HEAD(&memfile[i].page_cache_list);
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

