/*
 * Initialise the system.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <mm/alloc_page.h>
#include <lib/malloc.h>
#include <lib/bit.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/utcb.h>
#include <task.h>
#include <shm.h>
#include <file.h>
#include <init.h>
#include <test.h>
#include <boot.h>
#include <utcb.h>
#include <bootm.h>

/* A separate list than the generic file list that keeps just the boot files */
LINK_DECLARE(boot_file_list);

/* Kernel data acquired during initialisation */
__initdata struct initdata initdata;

void print_bootdesc(struct bootdesc *bd)
{
	for (int i = 0; i < bd->total_images; i++) {
		printf("Task Image: %d\n", i);
		printf("Name: %s\n", bd->images[i].name);
		printf("Start: 0x%x\n", bd->images[i].phys_start);
		printf("End: 0x%x\n", bd->images[i].phys_end);
	}
}

void print_pfn_range(int pfn, int size)
{
	unsigned int addr = pfn << PAGE_BITS;
	unsigned int end = (pfn + size) << PAGE_BITS;
	printf("Used: 0x%x - 0x%x\n", addr, end);
}

#if 0
void print_page_map(struct page_bitmap *map)
{
	unsigned int start_pfn = 0;
	unsigned int total_used = 0;
	int numpages = 0;

	// printf("Page map: 0x%x-0x%x\n", map->pfn_start << PAGE_BITS, map->pfn_end << PAGE_BITS);
	for (int i = 0; i < (PHYSMEM_TOTAL_PAGES >> 5); i++) {
		for (int x = 0; x < WORD_BITS; x++) {
			if (map->map[i] & (1 << x)) { /* A used page found? */
				if (!start_pfn) /* First such page found? */
					start_pfn = (WORD_BITS * i) + x;
				total_used++;
				numpages++; /* Increase number of pages */
			} else { /* Either used pages ended or were never found */
				if (start_pfn) { /* We had a used page */
					/* Finished end of used range.
					 * Print and reset. */
					//print_pfn_range(start_pfn, numpages);
					start_pfn = 0;
					numpages = 0;
				}
			}
		}
	}
	printf("%s: Pagemap: Total of %d used physical pages. %d Kbytes used.\n", __TASKNAME__, total_used, total_used << 2);
}
#endif

/*
 * A specialised function for setting up the task environment of mm0.
 * Essentially all the memory regions are set up but a new task isn't
 * created, registers aren't assigned, and thread not started, since
 * these are all already done by the kernel. But we do need a memory
 * environment for mm0, hence this function.
 */
int mm0_task_init(struct vm_file *f, unsigned long task_start,
		  unsigned long task_end, struct task_ids *ids)
{
	int err;
	struct tcb *task;

	/*
	 * The thread itself is already known by the kernel, so we just
	 * allocate a local task structure.
	 */
	BUG_ON(IS_ERR(task = tcb_alloc_init(TCB_NO_SHARING)));

	task->tid = ids->tid;
	task->spid = ids->spid;
	task->tgid = ids->tgid;

	if ((err = boottask_setup_regions(f, task,
					  task_start, task_end)) < 0)
		return err;

	if ((err =  boottask_mmap_regions(task, f)) < 0)
		return err;

	/* Set pager as child and parent of itself */
	list_insert(&task->child_ref, &task->children);
	task->parent = task;

	/*
	 * The first UTCB address is already assigned by the
	 * microkernel for this pager. Ensure that we also get
	 * the same from our internal utcb bookkeeping.
	 */
	BUG_ON(task->utcb_address != UTCB_AREA_START);

	/* Pager must prefault its utcb */
	prefault_page(task, task->utcb_address,
		      VM_READ | VM_WRITE);

	/* Add the task to the global task list */
	global_add_task(task);

	/* Add the file to global vm lists */
	global_add_vm_file(f);

	return 0;
}

struct vm_file *initdata_next_bootfile(struct initdata *initdata)
{
	struct vm_file *file, *n;
	list_foreach_removable_struct(file, n,
				      &initdata->boot_file_list,
				      list) {
		list_remove_init(&file->list);
		return file;
	}
	return 0;
}

/*
 * Reads boot files from init data, determines their task ids if they
 * match with particular servers, and starts the tasks.
 */
int start_boot_tasks(struct initdata *initdata)
{
	struct vm_file *file = 0, *fs0_file = 0, *mm0_file = 0, *n;
	struct tcb *fs0_task;
	struct svc_image *img;
	struct task_ids ids;
	struct link other_files;
	int total = 0;

	link_init(&other_files);

	/* Separate out special server tasks and regular files */
	do {
		file = initdata_next_bootfile(initdata);

		if (file) {
			BUG_ON(file->type != VM_FILE_BOOTFILE);
			img = file->priv_data;
			if (!strcmp(img->name, __PAGERNAME__))
				mm0_file = file;
			else if (!strcmp(img->name, __VFSNAME__))
				fs0_file = file;
			else
				list_insert(&file->list, &other_files);
		} else
			break;
	} while (1);

	/* MM0 needs partial initialisation since it's already running. */
	// printf("%s: Initialising mm0 tcb.\n", __TASKNAME__);
	ids.tid = PAGER_TID;
	ids.spid = PAGER_TID;
	ids.tgid = PAGER_TID;

	if (mm0_task_init(mm0_file, INITTASK_AREA_START,
			  INITTASK_AREA_END, &ids) < 0)
		BUG();
	total++;

	/* Initialise vfs with its predefined id */
	ids.tid = VFS_TID;
	ids.spid = VFS_TID;
	ids.tgid = VFS_TID;

	// printf("%s: Initialising fs0\n",__TASKNAME__);
	BUG_ON((IS_ERR(fs0_task = boottask_exec(fs0_file, USER_AREA_START,
						USER_AREA_END, &ids))));
	total++;

	/* Initialise other tasks */
	list_foreach_removable_struct(file, n, &other_files, list) {
		ids.tid = TASK_ID_INVALID;
		ids.spid = TASK_ID_INVALID;
		ids.tgid = TASK_ID_INVALID;
		list_remove_init(&file->list);
		BUG_ON(IS_ERR(boottask_exec(file, USER_AREA_START,
					    USER_AREA_END, &ids)));
		total++;
	}

	if (!total) {
		printf("%s: Could not start any tasks.\n",
		       __TASKNAME__);
		BUG();
	}

	return 0;
}

extern unsigned long _start_init[];
extern unsigned long _end_init[];

/*
 * Copy all necessary data from initmem to real memory,
 * release initdata and any init memory used
 */
void copy_release_initdata(struct initdata *initdata)
{
	/*
	 * Copy boot capabilities to a list of
	 * real capabilities
	 */
	copy_boot_capabilities(initdata);

	/* Free and unmap init memory:
	 *
	 * FIXME: We can and do safely unmap the boot
	 * memory here, but because we don't utilize it yet,
	 * it remains as if it is a used block
	 */

	l4_unmap(_start_init,
		 __pfn(page_align_up(_end_init - _start_init)),
		 self_tid());
}


void init_mm(struct initdata *initdata)
{
	/* Initialise the page and bank descriptors */
	init_physmem_primary(initdata);

	init_physmem_secondary(initdata, membank);

	/* Initialise the page allocator on first bank. */
	init_page_allocator(membank[0].free, membank[0].end);

	/* Initialise the zero page */
	init_devzero();

	/* Initialise in-memory boot files */
	init_boot_files(initdata);

	if (shm_pool_init() < 0) {
		printf("SHM initialisation failed.\n");
		BUG();
	}

	if (utcb_pool_init() < 0) {
		printf("SHM initialisation failed.\n");
		BUG();
	}
}


void init_pager(void)
{
	pager_address_pool_init();

	read_kernel_capabilities(&initdata);

	read_bootdesc(&initdata);

	init_mm(&initdata);

	start_boot_tasks(&initdata);

	copy_release_initdata(&initdata);

	mm0_test_global_vm_integrity();
}

