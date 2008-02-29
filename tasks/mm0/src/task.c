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
#include INC_GLUE(memory.h)
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <lib/addr.h>
#include <task.h>
#include <kdata.h>
#include <kmalloc/kmalloc.h>
#include <string.h>
#include <vm_area.h>
#include <memory.h>
#include <file.h>
#include <utcb.h>
#include <env.h>

struct tcb_head {
	struct list_head list;
	int total;			/* Total threads */
} tcb_head;

struct tcb *find_task(int tid)
{
	struct tcb *t;

	list_for_each_entry(t, &tcb_head.list, list)
		if (t->tid == tid)
			return t;
	return 0;
}

#if 0
void dump_tasks(void)
{
	struct tcb *t;

	list_for_each_entry(t, &tcb_head.list, list) {
		printf("Task %s: id/spid: %d/%d\n", &t->name[0], t->tid, t->spid);
		printf("Task vm areas:\n");
		dump_vm_areas(t);
		printf("Task swapfile:\n");
		dump_task_swapfile(t);
	}
}
#endif


struct tcb *create_init_tcb(struct tcb_head *tcbs)
{
	struct tcb *task = kzalloc(sizeof(struct tcb));

	/* Ids will be acquired from the kernel */
	task->tid = TASK_ID_INVALID;
	task->spid = TASK_ID_INVALID;
	task->swap_file = kzalloc(sizeof(struct vm_file));
	task->swap_file->pager = &swap_pager;
	address_pool_init(&task->swap_file_offset_pool, 0,
			  __pfn(TASK_SWAPFILE_MAXSIZE));
	INIT_LIST_HEAD(&task->swap_file->page_cache_list);
	INIT_LIST_HEAD(&task->list);
	INIT_LIST_HEAD(&task->vm_area_list);
	list_add_tail(&task->list, &tcbs->list);
	tcbs->total++;

	/* Allocate a utcb virtual address */
	task->utcb_address = (unsigned long)utcb_vaddr_new();

	return task;
}

int start_boot_tasks(struct initdata *initdata, struct tcb_head *tcbs)
{
	int err;
	struct vm_file *file;
	struct svc_image *img;
	unsigned int sp, pc;
	struct tcb *task;
	struct task_ids ids;
	struct bootdesc *bd = initdata->bootdesc;

	INIT_LIST_HEAD(&tcb_head.list);
	INIT_LIST_HEAD(&initdata->boot_file_list);

	for (int i = 0; i < bd->total_images; i++) {
		img = &bd->images[i];

		/* Skip self */
		if (!strcmp(img->name, __PAGERNAME__))
			continue;

		/* Set up task ids */
		if (!strcmp(img->name, __VFSNAME__)) {
			ids.tid = VFS_TID;
			ids.spid = VFS_TID;
		} else {
			ids.tid = -1;
			ids.spid = -1;
		}

		printf("Creating new thread.\n");
		/* Create the thread structures and address space */
		if ((err = l4_thread_control(THREAD_CREATE, &ids)) < 0) {
			printf("l4_thread_control failed with %d.\n", err);
			goto error;
		}

		/* Create a task and use returned space and thread ids. */
		printf("New task with id: %d, space id: %d\n", ids.tid, ids.spid);
		task = create_init_tcb(tcbs);
		task->tid = ids.tid;
		task->spid = ids.spid;

		/*
		 * For boot files, we use the physical address of the memory
		 * file as its mock-up inode.
		 */
		file = vmfile_alloc_init();
		file->vnum = img->phys_start;
		file->length = img->phys_end - img->phys_start;
		file->pager = &boot_file_pager;
		list_add(&file->list, &initdata->boot_file_list);

		/* Prepare environment boundaries. Posix minimum is 4Kb */
		task->env_end = USER_AREA_END;
		task->env_start = task->env_end - PAGE_SIZE;
		task->args_start = task->env_start;
		task->args_end = task->env_start;

		/*
		 * Prepare the task environment file and data.
		 * Currently it only has the utcb address. The env pager
		 * when faulted, simply copies the task env data to the
		 * allocated page.
		 */
		if (task_prepare_environment(task) < 0) {
			printf("Could not create environment file.\n");
			goto error;
		}

		/*
		 * Task stack starts right after the environment,
		 * and is of 4 page size.
		 */
		task->stack_end = task->env_start;
		task->stack_start = task->stack_end - PAGE_SIZE * 4;

		/* Only text start is valid */
		task->text_start = USER_AREA_START;

		/* Set up task's registers */
		sp = align(task->stack_end - 1, 8);
		pc = task->text_start;

		/* mmap each task's physical image to task's address space. */
		if ((err = do_mmap(file, 0, task, USER_AREA_START,
				   VM_READ | VM_WRITE | VM_EXEC,
				   __pfn(page_align_up(file->length)))) < 0) {
			printf("do_mmap: failed with %d.\n", err);
			goto error;
		}

		/* mmap each task's environment from its env file. */
		if ((err = do_mmap(task->env_file, 0, task, task->env_start,
				   VM_READ | VM_WRITE,
				   __pfn(task->env_end - task->env_start)) < 0)) {
			printf("do_mmap: Mapping environment failed with %d.\n",
			       err);
			goto error;
		}

		/* mmap each task's stack as 4-page anonymous memory. */
		if ((err = do_mmap(0, 0, task, task->stack_start,
				   VM_READ | VM_WRITE | VMA_ANON,
				   __pfn(task->stack_end - task->stack_start)) < 0)) {
			printf("do_mmap: Mapping stack failed with %d.\n", err);
			goto error;
		}

		/* mmap each task's utcb as single page anonymous memory. */
		printf("%s: Mapping utcb for new task at: 0x%x\n", __TASKNAME__,
		       task->utcb_address);
		if ((err = do_mmap(0, 0, task, task->utcb_address,
				   VM_READ | VM_WRITE | VMA_ANON, 1) < 0)) {
			printf("do_mmap: Mapping utcb failed with %d.\n", err);
			goto error;
		}

		/* Set up the task's thread details, (pc, sp, pager etc.) */
		if ((err = l4_exchange_registers(pc, sp, self_tid(), task->tid) < 0)) {
			printf("l4_exchange_registers failed with %d.\n", err);
			goto error;
		}

		printf("Starting task with id %d\n", task->tid);

		/* Start the thread */
		if ((err = l4_thread_control(THREAD_RUN, &ids) < 0)) {
			printf("l4_thread_control failed with %d\n", err);
			goto error;
		}
	}
	return 0;

error:
	BUG();
}


void init_pm(struct initdata *initdata)
{
	start_boot_tasks(initdata, &tcb_head);
}

/*
 * During its initialisation FS0 wants to learn how many boot tasks
 * are running, and their tids, which includes itself. This function
 * provides that information.
 */
void send_task_data(l4id_t requester)
{
	struct tcb *t;
	int li, err;

	if (requester != VFS_TID) {
		printf("%s: Task data requested by %d, which is not "
		       "FS0 id %d, ignoring.\n", __TASKNAME__, requester,
		       VFS_TID);
		return;
	}

	/* First word is total number of tcbs */
	write_mr(L4SYS_ARG0, tcb_head.total);

	/* Write each tcb's tid */
	li = 0;
	list_for_each_entry(t, &tcb_head.list, list) {
		BUG_ON(li >= MR_USABLE_TOTAL);
		write_mr(L4SYS_ARG1 + li, t->tid);
		li++;
	}

	/* Reply */
	if ((err = l4_ipc_return(0)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		BUG();
	}
}

