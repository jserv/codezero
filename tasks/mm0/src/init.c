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

/* A separate list than the generic file list that keeps just the boot files */
LIST_HEAD(boot_file_list);

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

	if ((err = task_setup_regions(f, task, task_start, task_end)) < 0)
		return err;

	if ((err =  task_mmap_regions(task, f)) < 0)
		return err;

	/* Add the task to the global task list */
	task_add_global(task);

	/* Add the file to global vm lists */
	list_del_init(&f->list);
	list_del_init(&f->vm_obj.list);
	list_add(&f->list, &vm_file_list);
	list_add(&f->vm_obj.list, &vm_object_list);

	return 0;
}

struct vm_file *initdata_next_bootfile(struct initdata *initdata)
{
	struct vm_file *file, *n;
	list_for_each_entry_safe(file, n, &initdata->boot_file_list,
				 list) {
		list_del_init(&file->list);
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
	struct vm_file *file = 0, *fs0 = 0, *mm0 = 0, *n;
	struct svc_image *img;
	struct task_ids ids;
	struct list_head files;
	int total = 0;

	INIT_LIST_HEAD(&files);

	/* Separate out special server tasks and regular files */
	do {
		file = initdata_next_bootfile(initdata);

		if (file) {
			BUG_ON(file->type != VM_FILE_BOOTFILE);
			img = file->priv_data;
			if (!strcmp(img->name, __PAGERNAME__))
				mm0 = file;
			else if (!strcmp(img->name, __VFSNAME__))
				fs0 = file;
			else
				list_add(&file->list, &files);
		} else
			break;
	} while (1);

	/* MM0 needs partial initialisation since it's already running. */
	printf("%s: Initialising mm0 tcb.\n", __TASKNAME__);
	ids.tid = PAGER_TID;
	ids.spid = PAGER_TID;
	ids.tgid = PAGER_TID;

	if (mm0_task_init(mm0, INITTASK_AREA_START, INITTASK_AREA_END, &ids) < 0)
		BUG();
	total++;

	/* Initialise vfs with its predefined id */
	ids.tid = VFS_TID;
	ids.spid = VFS_TID;
	ids.tgid = VFS_TID;

	printf("%s: Initialising fs0\n",__TASKNAME__);
	if (task_exec(fs0, USER_AREA_START, USER_AREA_END, &ids) < 0)
		BUG();
	total++;

	/* Initialise other tasks */
	list_for_each_entry_safe(file, n, &files, list) {
		printf("%s: Initialising new boot task.\n", __TASKNAME__);
		ids.tid = TASK_ID_INVALID;
		ids.spid = TASK_ID_INVALID;
		ids.tgid = TASK_ID_INVALID;
		if (task_exec(file, USER_AREA_START, USER_AREA_END, &ids) < 0)
			BUG();
		total++;
	}

	if (!total) {
		printf("%s: Could not start any tasks.\n", __TASKNAME__);
		BUG();
	}

	return 0;
}


void init_mm(struct initdata *initdata)
{
	/* Initialise the page and bank descriptors */
	init_physmem(initdata, membank);
	// printf("%s: Initialised physmem.\n", __TASKNAME__);

	/* Initialise the page allocator on first bank. */
	init_page_allocator(membank[0].free, membank[0].end);
	// printf("%s: Initialised page allocator.\n", __TASKNAME__);

	/* Initialise the pager's memory allocator */
	kmalloc_init();
	// printf("%s: Initialised kmalloc.\n", __TASKNAME__);

	/* Initialise the zero page */
	init_devzero();
	// printf("%s: Initialised devzero.\n", __TASKNAME__);

	/* Initialise in-memory boot files */
	init_boot_files(initdata);
	// printf("%s: Initialised in-memory boot files.\n", __TASKNAME__);

	shm_init();
	// printf("%s: Initialised shm structures.\n", __TASKNAME__);

	if (utcb_pool_init() < 0) {
		printf("UTCB initialisation failed.\n");
		BUG();
	}
	// printf("%s: Initialised utcb address pool.\n", __TASKNAME__);

	/* Give the kernel some memory to use for its allocators */
	l4_kmem_control(__pfn(alloc_page(__pfn(SZ_1MB))), __pfn(SZ_1MB), 1);
}

void initialise(void)
{
	request_initdata(&initdata);

	init_mm(&initdata);

	start_boot_tasks(&initdata);

	printf("%s: Initialised the memory/process manager.\n", __TASKNAME__);
}

