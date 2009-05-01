/*
 * Functions used for running initial tasks during boot.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <boot.h>
#include <mmap.h>
#include <shm.h>
#include <utcb.h>
#include <l4/api/thread.h>
#include <l4lib/arch/syslib.h>

int boottask_setup_regions(struct vm_file *file, struct tcb *task,
			   unsigned long task_start, unsigned long task_end)
{
	/*
	 * Set task's main address space boundaries. Not all tasks
	 * run in the default user boundaries, e.g. mm0 pager.
	 */
	task->start = task_start;
	task->end = task_end;

	/* Task stack starts right after the environment. */
	task->stack_end = task->end;
	task->stack_start = task->stack_end - DEFAULT_STACK_SIZE;

	/* Prepare environment boundaries. */
	task->args_end = task->stack_end;
	task->args_start = task->stack_start - DEFAULT_ENV_SIZE;

	/* Currently RO text and RW data are one region. TODO: Fix this */
	task->data_start = task->start;
	task->data_end = task->start + page_align_up(file->length);
	task->text_start = task->data_start;
	task->text_end = task->data_end;
	task->entry = task->text_start;

	/* Task's region available for mmap */
	task->map_start = task->data_end;
	task->map_end = task->stack_start;

	return 0;
}

/*
 * Used for mmapping boot task regions. These are slightly different
 * from a vfs executable file.
 */
int boottask_mmap_regions(struct tcb *task, struct vm_file *file)
{
	void *mapped;
	struct vm_file *shm;

	/*
	 * mmap each task's physical image to task's address space.
	 * TODO: Map data and text separately when available from bootdesc.
	 */
	if (IS_ERR(mapped = do_mmap(file, 0, task, task->text_start,
				    VM_READ | VM_WRITE | VM_EXEC | VMA_PRIVATE,
				    __pfn(page_align_up(task->text_end) -
				    task->text_start)))) {
		printf("do_mmap: failed with %d.\n", (int)mapped);
		return (int)mapped;
	}

	/* mmap each task's stack as anonymous memory. */
	if (IS_ERR(mapped = do_mmap(0, 0, task, task->stack_start,
				    VM_READ | VM_WRITE |
				    VMA_PRIVATE | VMA_ANONYMOUS,
				    __pfn(task->stack_end -
					  task->stack_start)))) {
		printf("do_mmap: Mapping stack failed with %d.\n",
		       (int)mapped);
		return (int)mapped;
	}

	/* Task's default shared page */
	task->shared_page = shm_new_address(DEFAULT_SHPAGE_SIZE/PAGE_SIZE);

	/* Create a shared memory segment available for shmat() */
	if (IS_ERR(shm = shm_new((key_t)task->shared_page,
				 __pfn(DEFAULT_SHPAGE_SIZE))))
		return (int)shm;

	/* Task's utcb region */
	task_setup_utcb(task);

	return 0;
}

/*
 * Main entry point for the creation, initialisation and
 * execution of a new task.
 */
struct tcb *boottask_exec(struct vm_file *f, unsigned long task_region_start,
			  unsigned long task_region_end, struct task_ids *ids)
{
	struct tcb *task;
	int err;

	if (IS_ERR(task = task_create(0, ids, THREAD_NEW_SPACE,
				      TCB_NO_SHARING)))
		return task;

	if ((err = boottask_setup_regions(f, task, task_region_start,
					  task_region_end)) < 0)
		return PTR_ERR(err);

	if ((err = boottask_mmap_regions(task, f)) < 0)
		return PTR_ERR(err);

	if ((err =  task_setup_registers(task, 0, 0, 0)) < 0)
		return PTR_ERR(err);

	/* Add the task to the global task list */
	global_add_task(task);

	/* Add the file to global vm lists */
	global_add_vm_file(f);

	/* Prefault all its regions */
	if (ids->tid == VFS_TID)
		task_prefault_regions(task, f);

	/* Start the task */
	if ((err = task_start(task)) < 0)
		return PTR_ERR(err);

	return task;
}


