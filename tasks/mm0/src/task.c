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

/* Create temporary run-time files in memory to test with mmap */
struct vm_file *create_init_vmfile(struct list_head *vmfile_head)
{
	struct vm_file *file = kzalloc(sizeof(*file));

	INIT_LIST_HEAD(&file->list);
	INIT_LIST_HEAD(&file->page_cache_list);
	list_add(&file->list, vmfile_head);

	return file;
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

		/* Set up task's registers */
		sp = align(USER_AREA_END - 1, 8);
		pc = USER_AREA_START;

		/* Create vm file and tcb */
		file = create_init_vmfile(&initdata->boot_file_list);
		task = create_init_tcb(tcbs);

		/*
		 * For boot files, we use the physical address of the memory
		 * file as its mock-up inode.
		 */
		file->inode.i_addr = img->phys_start;
		file->length = img->phys_end - img->phys_start;
		file->pager = &default_file_pager;

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
			printf("do_mmap: Mapping utcb failed with %d.\n", err);
			goto error;
		}

		printf("Creating new thread.\n");
		/* Create the thread structures and address space */
		if ((err = l4_thread_control(THREAD_CREATE, &ids)) < 0) {
			printf("l4_thread_control failed with %d.\n", err);
			goto error;
		}

		/* Use returned space and thread ids. */
		printf("New task with id: %d, space id: %d\n", ids.tid, ids.spid);
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
		printf("%s: Task data requested by %d, which is not "
		       "FS0 id %d, ignoring.\n", __TASKNAME__, requester,
		       VFS_TID);
		return;
	}

	/* First word is total number of tcbs */
	write_mr(tcb_head.total, L4SYS_ARG0);

	/* Write each tcb's tid */
	li = 0;
	list_for_each_entry(t, &tcb_head.list, list) {
		BUG_ON(li >= MR_USABLE_TOTAL);
		write_mr(t->tid, L4SYS_ARG1 + li);
		li++;
	}

	/* Reply */
	if ((err = l4_ipc_return(0)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		BUG();
	}
}

