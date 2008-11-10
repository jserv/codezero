/*
 * Task management.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/macros.h>
#include <l4/config.h>
#include <l4/types.h>
#include <l4/lib/list.h>
#include <l4/api/thread.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include INC_GLUE(memory.h)
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/utcb.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/exregs.h>
#include <lib/addr.h>
#include <kmalloc/kmalloc.h>
#include <init.h>
#include <string.h>
#include <vm_area.h>
#include <memory.h>
#include <file.h>
#include <utcb.h>
#include <task.h>
#include <shm.h>
#include <mmap.h>
#include <globals.h>

struct global_list global_tasks = {
	.list = { &global_tasks.list, &global_tasks.list },
	.total = 0,
};

void print_tasks(void)
{
	struct tcb *task;
	printf("Tasks:\n========\n");
	list_for_each_entry(task, &global_tasks.list, list) {
		printf("Task tid: %d, spid: %d\n", task->tid, task->spid);
	}
}

void global_add_task(struct tcb *task)
{
	BUG_ON(!list_empty(&task->list));
	list_add_tail(&task->list, &global_tasks.list);
	global_tasks.total++;
}

void global_remove_task(struct tcb *task)
{
	BUG_ON(list_empty(&task->list));
	list_del_init(&task->list);
	BUG_ON(--global_tasks.total < 0);
}

struct tcb *find_task(int tid)
{
	struct tcb *t;

	list_for_each_entry(t, &global_tasks.list, list)
		if (t->tid == tid)
			return t;
	return 0;
}

struct tcb *tcb_alloc_init(unsigned int flags)
{
	struct tcb *task;

	if (!(task = kzalloc(sizeof(struct tcb))))
		return PTR_ERR(-ENOMEM);

	/* Allocate new vma head if its not shared */
	if (!(flags & TCB_SHARED_VM)) {
		if (!(task->vm_area_head =
		      kzalloc(sizeof(*task->vm_area_head)))) {
			kfree(task);
			return PTR_ERR(-ENOMEM);
		}
		task->vm_area_head->tcb_refs = 1;
		INIT_LIST_HEAD(&task->vm_area_head->list);
	}

	/* Allocate file structures if not shared */
	if (!(flags & TCB_SHARED_FILES)) {
		if (!(task->files =
		      kzalloc(sizeof(*task->files)))) {
			kfree(task->vm_area_head);
			kfree(task);
			return PTR_ERR(-ENOMEM);
		}
		task->files->tcb_refs = 1;
	}

	/* Ids will be acquired from the kernel */
	task->tid = TASK_ID_INVALID;
	task->spid = TASK_ID_INVALID;
	task->tgid = TASK_ID_INVALID;

	/* Initialise list structure */
	INIT_LIST_HEAD(&task->list);

	return task;
}

/*
 * Free vmas, fd structure and utcb address.
 * Make sure to sync all IO beforehand
 */
int task_free_resources(struct tcb *task)
{
	/*
	 * Threads may share file descriptor structure
	 * if no users left, free it.
	 */
	if (!(--task->files->tcb_refs))
		kfree(task->files);

	/*
	 * Threads may share the virtual space.
	 * if no users of the vma struct left,
	 * free it along with all its vma links.
	 */
	if (!(--task->vm_area_head->tcb_refs)) {
		/* Free all vmas */
		task_release_vmas(task->vm_area_head);

		/* Free the head */
		kfree(task->vm_area_head);
	}

	return 0;
}

int tcb_destroy(struct tcb *task)
{
	global_remove_task(task);

	/* Free all resources of the task */
	task_free_resources(task);

	kfree(task);

	return 0;
}

/*
 * Copy all vmas from the given task and populate each with
 * links to every object that the original vma is linked to.
 * Note, that we don't copy vm objects but just the links to
 * them, because vm objects are not per-process data.
 */
int copy_vmas(struct tcb *to, struct tcb *from)
{
	struct vm_area *vma, *new_vma;

	list_for_each_entry(vma, &from->vm_area_head->list, list) {

		/* Create a new vma */
		new_vma = vma_new(vma->pfn_start, vma->pfn_end - vma->pfn_start,
				  vma->flags, vma->file_offset);

		/* Copy all object links */
		vma_copy_links(new_vma, vma);

		/* All link copying is finished, now add the new vma to task */
		task_add_vma(to, new_vma);
	}

	return 0;
}

/*
 * Traverse all vmas, release all links to vm_objects.
 * Used when a task or thread group with a shared vm is exiting.
 */
int task_release_vmas(struct task_vma_head *vma_head)
{
	struct vm_area *vma, *n;

	list_for_each_entry_safe(vma, n, &vma_head->list, list) {
		/* Release all links */
		vma_drop_merge_delete_all(vma);

		/* Delete the vma from task's vma list */
		list_del(&vma->list);

		/* Free the vma */
		kfree(vma);
	}
	return 0;
}

int copy_tcb(struct tcb *to, struct tcb *from, unsigned int flags)
{
	/* Copy program segment boundary information */
	to->start = from->start;
	to->end = from->end;
	to->text_start = from->text_start;
	to->text_end = from->text_end;
	to->data_start = from->data_start;
	to->data_end = from->data_end;
	to->bss_start = from->bss_start;
	to->bss_end = from->bss_end;
	to->stack_start = from->stack_start;
	to->stack_end = from->stack_end;
	to->heap_start = from->heap_start;
	to->heap_end = from->heap_end;
	to->env_start = from->env_start;
	to->env_end = from->env_end;
	to->args_start = from->args_start;
	to->args_end = from->args_end;
	to->map_start = from->map_start;
	to->map_end = from->map_end;

	/* Sharing the list of vmas */
	if (flags & TCB_SHARED_VM) {
		to->vm_area_head = from->vm_area_head;
		to->vm_area_head->tcb_refs++;
	} else {
	       	/* Copy all vm areas */
		copy_vmas(to, from);
	}

	/* Copy all file descriptors */
	if (flags & TCB_SHARED_FILES) {
		to->files = from->files;
		to->files->tcb_refs++;
	} else {
	       	/* Bulk copy all file descriptors */
		memcpy(to->files, from->files, sizeof(*to->files));

		/* Increase refcount for all open files */
		for (int i = 0; i < TASK_FILES_MAX; i++)
			if (to->files->fd[i].vmfile)
				to->files->fd[i].vmfile->openers++;
	}

	return 0;
}

struct tcb *task_create(struct tcb *orig, struct task_ids *ids,
			unsigned int ctrl_flags, unsigned int share_flags)
{
	struct tcb *task;
	int err;

	/* Create the thread structures and address space */
	if ((err = l4_thread_control(THREAD_CREATE | ctrl_flags, ids)) < 0) {
		printf("l4_thread_control failed with %d.\n", err);
		return PTR_ERR(err);
	}

	/* Create a task and use given space and thread ids. */
	if (IS_ERR(task = tcb_alloc_init(share_flags)))
		return PTR_ERR(task);

	/* Set task's ids */
	task->tid = ids->tid;
	task->spid = ids->spid;
	task->tgid = ids->tgid;

	/*
	 * If an original task has been specified, that means either
	 * we are forking, or we are cloning the original tcb fully
	 * or partially. Therefore we copy tcbs depending on share flags.
	 */
	if (orig)
		copy_tcb(task, orig, share_flags);

	return task;
}


int task_mmap_regions(struct tcb *task, struct vm_file *file)
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

	/* mmap each task's environment as anonymous memory. */
	if (IS_ERR(mapped = do_mmap(0, 0, task, task->env_start,
				    VM_READ | VM_WRITE |
				    VMA_PRIVATE | VMA_ANONYMOUS,
				    __pfn(task->env_end - task->env_start)))) {
		printf("do_mmap: Mapping environment failed with %d.\n",
		       (int)mapped);
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

	/* Task's utcb */
	task->utcb = utcb_new_address();

	/* Create a shared memory segment available for shmat() */
	if (IS_ERR(shm = shm_new((key_t)task->utcb, __pfn(DEFAULT_UTCB_SIZE))))
		return (int)shm;

	return 0;
}

int task_setup_regions(struct vm_file *file, struct tcb *task,
		       unsigned long task_start, unsigned long task_end)
{
	/*
	 * Set task's main address space boundaries. Not all tasks
	 * run in the default user boundaries, e.g. mm0 pager.
	 */
	task->start = task_start;
	task->end = task_end;

	/* Prepare environment boundaries. */
	task->env_end = task->end;
	task->env_start = task->env_end - DEFAULT_ENV_SIZE;
	task->args_end = task->env_start;
	task->args_start = task->env_start;

	/* Task stack starts right after the environment. */
	task->stack_end = task->env_start;
	task->stack_start = task->stack_end - DEFAULT_STACK_SIZE;

	/* Currently RO text and RW data are one region. TODO: Fix this */
	task->data_start = task->start;
	task->data_end = task->start + page_align_up(file->length);
	task->text_start = task->data_start;
	task->text_end = task->data_end;

	/* Task's region available for mmap */
	task->map_start = task->data_end;
	task->map_end = task->stack_start;

	return 0;
}

int task_setup_registers(struct tcb *task, unsigned int pc,
			 unsigned int sp, l4id_t pager)
{
	int err;
	struct exregs_data exregs;

	/* Set up task's registers to default. */
	if (!sp)
		sp = align(task->stack_end - 1, 8);
	if (!pc)
		pc = task->text_start;
	if (!pager)
		pager = self_tid();

	/* Set up the task's thread details, (pc, sp, pager etc.) */
	exregs_set_stack(&exregs, sp);
	exregs_set_pc(&exregs, pc);
	exregs_set_pager(&exregs, pager);

	if ((err = l4_exchange_registers(&exregs, task->tid) < 0)) {
		printf("l4_exchange_registers failed with %d.\n", err);
		return err;
	}

	return 0;
}

int task_start(struct tcb *task, struct task_ids *ids)
{
	int err;

	/* Start the thread */
	printf("Starting task with id %d, spid: %d\n", task->tid, task->spid);
	if ((err = l4_thread_control(THREAD_RUN, ids)) < 0) {
		printf("l4_thread_control failed with %d\n", err);
		return err;
	}

	return 0;
}

/*
 * Prefaults all mapped regions of a task. The reason we have this is
 * some servers are in the page fault handling path (e.g. fs0), and we
 * don't want them to fault and cause deadlocks and circular deps.
 *
 * Normally fs0 faults dont cause dependencies because its faults
 * are handled by the boot pager, which is part of mm0. BUT: It may
 * cause deadlocks because fs0 may fault while serving a request
 * from mm0.(Which is expected to also handle the fault).
 */
int task_prefault_regions(struct tcb *task, struct vm_file *f)
{
	struct vm_area *vma;

	list_for_each_entry(vma, &task->vm_area_head->list, list) {
		for (int pfn = vma->pfn_start; pfn < vma->pfn_end; pfn++)
			BUG_ON(prefault_page(task, __pfn_to_addr(pfn),
					     VM_READ | VM_WRITE) < 0);
	}
	return 0;
}

/*
 * Main entry point for the creation, initialisation and
 * execution of a new task.
 */
struct tcb *task_exec(struct vm_file *f, unsigned long task_region_start,
		      unsigned long task_region_end, struct task_ids *ids)
{
	struct tcb *task;
	int err;

	if (IS_ERR(task = task_create(0, ids, THREAD_NEW_SPACE,
				      TCB_NO_SHARING)))
		return task;

	if ((err = task_setup_regions(f, task, task_region_start,
				      task_region_end)) < 0)
		return PTR_ERR(err);

	if ((err = task_mmap_regions(task, f)) < 0)
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
	if ((err = task_start(task, ids)) < 0)
		return PTR_ERR(err);

	return task;
}

/*
 * During its initialisation FS0 wants to learn how many boot tasks
 * are running, and their tids, which includes itself. This function
 * provides that information.
 */
int send_task_data(struct tcb *vfs)
{
	int li = 0;
	struct tcb *t, *self;
	struct task_data_head *tdata_head;

	if (vfs->tid != VFS_TID) {
		printf("%s: Task data requested by %d, which is not "
		       "FS0 id %d, ignoring.\n", __TASKNAME__, vfs->tid,
		       VFS_TID);
		return 0;
	}

	BUG_ON(!(self = find_task(self_tid())));
	BUG_ON(!vfs->utcb);

	/* Attach mm0 to vfs's utcb segment just like a normal task */
	utcb_map_to_task(vfs, self, UTCB_PREFAULT);

	/* Write all requested task information to utcb's user buffer area */
	tdata_head = (struct task_data_head *)vfs->utcb;

	/* First word is total number of tcbs */
	tdata_head->total = global_tasks.total;

	/* Write per-task data for all tasks */
	list_for_each_entry(t, &global_tasks.list, list) {
		tdata_head->tdata[li].tid = t->tid;
		tdata_head->tdata[li].utcb_address = (unsigned long)t->utcb;
		li++;
	}

	return 0;
}

