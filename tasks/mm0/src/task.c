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

/* A separate list than the generic file list that keeps just the boot files */
LIST_HEAD(boot_file_list);

struct tcb_head {
	struct list_head list;
	int total;			/* Total threads */
} tcb_head;

void print_tasks(void)
{
	struct tcb *task;
	printf("Tasks:\n========\n");
	list_for_each_entry(task, &tcb_head.list, list) {
		printf("Task tid: %d, spid: %d\n", task->tid, task->spid);
	}
}

struct tcb *find_task(int tid)
{
	struct tcb *t;

	list_for_each_entry(t, &tcb_head.list, list) {
		/* A temporary precaution */
		BUG_ON(t->tid != t->spid);
		if (t->tid == tid) {
			return t;
		}
	}
	return 0;
}

struct tcb *tcb_alloc_init(void)
{
	struct tcb *task;

	if (!(task = kzalloc(sizeof(struct tcb))))
		return PTR_ERR(-ENOMEM);

	/* Ids will be acquired from the kernel */
	task->tid = TASK_ID_INVALID;
	task->spid = TASK_ID_INVALID;

	/* Initialise its lists */
	INIT_LIST_HEAD(&task->list);
	INIT_LIST_HEAD(&task->vm_area_list);

	return task;
}


struct tcb *task_create(struct task_ids *ids)
{
	struct tcb *task;
	int err;

	/* Create the thread structures and address space */
	if ((err = l4_thread_control(THREAD_CREATE, ids)) < 0) {
		printf("l4_thread_control failed with %d.\n", err);
		return PTR_ERR(err);
	}

	/* Create a task and use given space and thread ids. */
	if (IS_ERR(task = tcb_alloc_init()))
		return PTR_ERR(task);

	task->tid = ids->tid;
	task->spid = ids->spid;

	return task;
}


int task_mmap_regions(struct tcb *task, struct vm_file *file)
{
	int err;
	struct vm_file *shm;

	/*
	 * mmap each task's physical image to task's address space.
	 * TODO: Map data and text separately when available from bootdesc.
	 */
	if ((err = do_mmap(file, 0, task, task->text_start,
			   VM_READ | VM_WRITE | VM_EXEC | VMA_PRIVATE,
			   __pfn(page_align_up(task->text_end) -
			   task->text_start))) < 0) {
		printf("do_mmap: failed with %d.\n", err);
		return err;
	}

	/* mmap each task's environment as anonymous memory. */
	if ((err = do_mmap(0, 0, task, task->env_start,
			   VM_READ | VM_WRITE | VMA_PRIVATE | VMA_ANONYMOUS,
			   __pfn(task->env_end - task->env_start))) < 0) {
		printf("do_mmap: Mapping environment failed with %d.\n",
		       err);
		return err;
	}

	/* mmap each task's stack as anonymous memory. */
	if ((err = do_mmap(0, 0, task, task->stack_start,
			   VM_READ | VM_WRITE | VMA_PRIVATE | VMA_ANONYMOUS,
			   __pfn(task->stack_end - task->stack_start))) < 0) {
		printf("do_mmap: Mapping stack failed with %d.\n", err);
		return err;
	}

	/* Task's utcb */
	task->utcb = utcb_vaddr_new();

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

	/* Set up task's registers to default. */
	if (!sp)
		sp = align(task->stack_end - 1, 8);
	if (!pc)
		pc = task->text_start;
	if (!pager)
		pager = self_tid();

	/* Set up the task's thread details, (pc, sp, pager etc.) */
	if ((err = l4_exchange_registers(pc, sp, pager, task->tid) < 0)) {
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
	BUG_ON(IS_ERR(task = tcb_alloc_init()));

	task->tid = ids->tid;
	task->spid = ids->spid;

	if ((err = task_setup_regions(f, task, task_start, task_end)) < 0)
		return err;

	if ((err =  task_mmap_regions(task, f)) < 0)
		return err;

	/* Add the task to the global task list */
	list_add_tail(&task->list, &tcb_head.list);
	tcb_head.total++;

	/* Add the file to global vm lists */
	list_del_init(&f->list);
	list_del_init(&f->vm_obj.list);
	list_add(&f->list, &vm_file_list);
	list_add(&f->vm_obj.list, &vm_object_list);

	return 0;
}

/*
 * Main entry point for the creation, initialisation and
 * execution of a new task.
 */
int task_exec(struct vm_file *f, unsigned long task_region_start,
	      unsigned long task_region_end, struct task_ids *ids)
{
	struct tcb *task;
	int err;

	if (IS_ERR(task = task_create(ids)))
		return (int)task;

	if ((err = task_setup_regions(f, task, task_region_start,
				      task_region_end)) < 0)
		return err;

	if ((err = task_mmap_regions(task, f)) < 0)
		return err;

	if ((err =  task_setup_registers(task, 0, 0, 0)) < 0)
		return err;

	/* Add the task to the global task list */
	list_add_tail(&task->list, &tcb_head.list);
	tcb_head.total++;

	/* Add the file to global vm lists */
	list_del_init(&f->list);
	list_del_init(&f->vm_obj.list);
	list_add(&f->list, &vm_file_list);
	list_add(&f->vm_obj.list, &vm_object_list);

	if ((err = task_start(task, ids)) < 0)
		return err;

	return 0;
}

#if 0
/*
 * Creates a process environment, mmaps the given file along
 * with any other necessary segment, and executes it as a task.
 */
int start_boot_task(struct vm_file *file, unsigned long task_start,
		    unsigned long task_end, struct task_ids *ids)
{
	int err;
	struct tcb *task;
	unsigned int sp, pc;

	/* Create the thread structures and address space */
	printf("Creating new thread.\n");
	if ((err = l4_thread_control(THREAD_CREATE, ids)) < 0) {
		printf("l4_thread_control failed with %d.\n", err);
		goto error;
	}

	/* Create a task and use given space and thread ids. */
	printf("New task with id: %d, space id: %d\n", ids->tid, ids->spid);
	task = tcb_alloc_init();
	task->tid = ids->tid;
	task->spid = ids->spid;

	/* Prepare environment boundaries. */
	task->env_end = task_end;
	task->env_start = task->env_end - DEFAULT_ENV_SIZE;
	task->args_end = task->env_start;
	task->args_start = task->env_start;

	/* Task stack starts right after the environment. */
	task->stack_end = task->env_start;
	task->stack_start = task->stack_end - DEFAULT_STACK_SIZE;

	/* Currently RO text and RW data are one region. TODO: Fix this */
	task->data_start = task_start;
	task->data_end = task_start + page_align_up(file->length);
	task->text_start = task->data_start;
	task->text_end = task->data_end;

	/* Task's region available for mmap */
	task->map_start = task->data_end;
	task->map_end = task->stack_start;

	/* Task's utcb */
	task->utcb = utcb_vaddr_new();

	/* Create a shared memory segment available for shmat() */
	shm_new((key_t)task->utcb, __pfn(DEFAULT_UTCB_SIZE));

	/* Set up task's registers */
	sp = align(task->stack_end - 1, 8);
	pc = task->text_start;

	/* Set up the task's thread details, (pc, sp, pager etc.) */
	if ((err = l4_exchange_registers(pc, sp, self_tid(), task->tid) < 0)) {
		printf("l4_exchange_registers failed with %d.\n", err);
		goto error;
	}

	/*
	 * mmap each task's physical image to task's address space.
	 * TODO: Map data and text separately when available from bootdesc.
	 */
	if ((err = do_mmap(file, 0, task, task->text_start,
			   VM_READ | VM_WRITE | VM_EXEC | VMA_PRIVATE,
			   __pfn(page_align_up(task->text_end) -
			   task->text_start))) < 0) {
		printf("do_mmap: failed with %d.\n", err);
		goto error;
	}

	/* mmap each task's environment as anonymous memory. */
	if ((err = do_mmap(0, 0, task, task->env_start,
			   VM_READ | VM_WRITE | VMA_PRIVATE | VMA_ANONYMOUS,
			   __pfn(task->env_end - task->env_start))) < 0) {
		printf("do_mmap: Mapping environment failed with %d.\n",
		       err);
		goto error;
	}

	/* mmap each task's stack as anonymous memory. */
	if ((err = do_mmap(0, 0, task, task->stack_start,
			   VM_READ | VM_WRITE | VMA_PRIVATE | VMA_ANONYMOUS,
			   __pfn(task->stack_end - task->stack_start))) < 0) {
		printf("do_mmap: Mapping stack failed with %d.\n", err);
		goto error;
	}

	/* Add the task to the global task list */
	list_add(&task->list, &tcb_head.list);
	tcb_head.total++;

	/* Start the thread */
	printf("Starting task with id %d\n", task->tid);
	if ((err = l4_thread_control(THREAD_RUN, ids)) < 0) {
		printf("l4_thread_control failed with %d\n", err);
		goto error;
	}

	return 0;

error:
	BUG();
}
#endif

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

	INIT_LIST_HEAD(&tcb_head.list);
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

	/* MM0 needs partial initialisation, since its already running. */
	printf("%s: Initialising mm0 tcb.\n", __TASKNAME__);
	ids.tid = PAGER_TID;
	ids.spid = PAGER_TID;
	if (mm0_task_init(mm0, INITTASK_AREA_START, INITTASK_AREA_END, &ids) < 0)
		BUG();
	total++;

	/* Initialise vfs with its predefined id */
	ids.tid = VFS_TID;
	ids.spid = VFS_TID;
	printf("%s: Initialising fs0\n",__TASKNAME__);
	if (task_exec(fs0, USER_AREA_START, USER_AREA_END, &ids) < 0)
		BUG();
	total++;

	/* Initialise other tasks */
	list_for_each_entry_safe(file, n, &files, list) {
		printf("%s: Initialising new boot task.\n", __TASKNAME__);
		ids.tid = TASK_ID_INVALID;
		ids.spid = TASK_ID_INVALID;
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

void init_pm(struct initdata *initdata)
{
	start_boot_tasks(initdata);
}

struct task_data {
	unsigned long tid;
	unsigned long utcb_address;
};

struct task_data_head {
	unsigned long total;
	struct task_data tdata[];
};

/*
 * During its initialisation FS0 wants to learn how many boot tasks
 * are running, and their tids, which includes itself. This function
 * provides that information.
 */
int send_task_data(l4id_t requester)
{
	int li = 0, err;
	struct tcb *t, *vfs, *self;
	struct task_data_head *tdata_head;

	if (requester != VFS_TID) {
		printf("%s: Task data requested by %d, which is not "
		       "FS0 id %d, ignoring.\n", __TASKNAME__, requester,
		       VFS_TID);
		return 0;
	}

	BUG_ON(!(vfs = find_task(requester)));
	BUG_ON(!(self = find_task(self_tid())));
	BUG_ON(!vfs->utcb);

	/* Attach mm0 to vfs's utcb segment just like a normal task */
	BUG_ON(IS_ERR(shmat_shmget_internal((key_t)vfs->utcb, vfs->utcb)));

	/* Prefault those pages to self. */
	for (int i = 0; i < __pfn(DEFAULT_UTCB_SIZE); i++)
		prefault_page(self, (unsigned long)vfs->utcb + __pfn_to_addr(i),
			      VM_READ | VM_WRITE);

	/* Write all requested task information to utcb's user buffer area */
	tdata_head = (struct task_data_head *)vfs->utcb;

	/* First word is total number of tcbs */
	tdata_head->total = tcb_head.total;

	/* Write per-task data for all tasks */
	list_for_each_entry(t, &tcb_head.list, list) {
		tdata_head->tdata[li].tid = t->tid;
		tdata_head->tdata[li].utcb_address = (unsigned long)t->utcb;
		li++;
	}

	/* Reply */
	if ((err = l4_ipc_return(0)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		BUG();
	}

	return 0;
}

