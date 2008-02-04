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
#include INC_GLUE(memory.h)
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <lib/vaddr.h>
#include <task.h>
#include <kdata.h>
#include <kmalloc/kmalloc.h>
#include <string.h>
#include <vm_area.h>
#include <memory.h>


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
	vaddr_pool_init(task->swap_file_offset_pool, 0,
		       	__pfn(TASK_SWAPFILE_MAXSIZE));
	INIT_LIST_HEAD(&task->swap_file->page_cache_list);
	INIT_LIST_HEAD(&task->list);
	INIT_LIST_HEAD(&task->vm_area_list);
	list_add_tail(&task->list, &tcbs->list);
	tcbs->total++;

	return task;
}

int start_boot_tasks(struct initdata *initdata, struct tcb_head *tcbs)
{
	struct vm_file *file;
	int err;

	INIT_LIST_HEAD(&tcb_head.list);
	list_for_each_entry(file, &initdata->boot_file_list, list) {
		struct tcb *task = create_init_tcb(tcbs);
		unsigned int sp = align(USER_AREA_END - 1, 8);
		unsigned int pc = USER_AREA_START;
		struct task_ids ids = { .tid = task->tid, .spid = task->spid };

		/* mmap each task's physical image to task's address space. */
		if ((err = do_mmap(file, 0, task, USER_AREA_START,
				   VM_READ | VM_WRITE | VM_EXEC,
				   __pfn(page_align_up(file->length)))) < 0) {
			printf("do_mmap: failed with %d.\n", err);
			goto error;
		}

		/* mmap each task's stack as single page anonymous memory. */
		if ((err = do_mmap(0, 0, task, USER_AREA_END - PAGE_SIZE,
				   VM_READ | VM_WRITE | VMA_ANON, 1) < 0)) {
			printf("do_mmap: Mapping stack failed with %d.\n", err);
			goto error;
		}

		/* mmap each task's utcb as single page anonymous memory. */
		if ((err = do_mmap(0, 0, task, (unsigned long)__L4_ARM_Utcb(),
				   VM_READ | VM_WRITE | VMA_ANON, 1) < 0)) {
			printf("do_mmap: Mapping stack failed with %d.\n", err);
			goto error;
		}

		printf("Creating new thread.\n");
		/* Create the thread structures and address space */
		if ((err = l4_thread_control(THREAD_CREATE, &ids)) < 0) {
			printf("l4_thread_control failed with %d.\n", err);
			goto error;
		}

		printf("New task with id: %d, space id: %d\n", ids.tid, ids.spid);
		/* Use returned space and thread ids. */
		task->tid = ids.tid;
		task->spid = ids.spid;

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
		printf("Task data is not requested by FS0, ignoring.\n");
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

