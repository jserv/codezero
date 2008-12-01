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
#include <l4/lib/math.h>
#include <lib/addr.h>
#include <lib/malloc.h>
#include <init.h>
#include <string.h>
#include <vm_area.h>
#include <memory.h>
#include <file.h>
#include <utcb.h>
#include <task.h>
#include <exec.h>
#include <shm.h>
#include <mmap.h>
#include <boot.h>
#include <globals.h>
#include <test.h>

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
	INIT_LIST_HEAD(&task->child_ref);
	INIT_LIST_HEAD(&task->children);

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
	struct tcb *child, *n;

	global_remove_task(task);

	/* Free all resources of the task */
	task_free_resources(task);

	/*
	 * All children of the current task becomes children
	 * of the parent of this task.
	 */
	list_for_each_entry_safe(child, n, &task->children,
				 child_ref) {
		list_del_init(&child->child_ref);
		list_add_tail(&child->child_ref,
			      &task->parent->children);
		child->parent = task->parent;
	}
	/* The task is not a child of its parent */
	list_del_init(&task->child_ref);

	/* Now task deletion make sure task is in no list */
	BUG_ON(!list_empty(&task->list));
	BUG_ON(!list_empty(&task->child_ref));
	BUG_ON(!list_empty(&task->children));
	kfree(task);

	return 0;
}

/*
 * Copy all vmas from the given task and populate each with
 * links to every object that the original vma is linked to.
 * Note, that we don't copy vm objects but just the links to
 * them, because vm objects are not per-process data.
 */
int task_copy_vmas(struct tcb *to, struct tcb *from)
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
		task_copy_vmas(to, from);
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

struct tcb *task_create(struct tcb *parent, struct task_ids *ids,
			unsigned int ctrl_flags, unsigned int share_flags)
{
	struct tcb *task;
	int err;

	/* Set task ids if a parent is supplied */
	if (parent) {
		ids->tid = TASK_ID_INVALID;
		ids->spid = parent->spid;

		/*
		 * Determine whether the cloned thread
		 * is in parent's thread group
		 */
		if (share_flags & TCB_SHARED_TGROUP)
			ids->tgid = parent->tgid;
		else
			ids->tgid = TASK_ID_INVALID;
	}

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

	/* Set task's creation flags */
	task->clone_flags = share_flags;

	/*
	 * If a parent task has been specified, that means either
	 * we are forking, or we are cloning the original tcb fully
	 * or partially. Therefore we copy tcbs depending on share flags.
	 */
	if (parent) {
		copy_tcb(task, parent, share_flags);

		/* Set up parent-child relationship */
		if ((share_flags & TCB_SHARED_PARENT) ||
		    (share_flags & TCB_SHARED_TGROUP)) {

			/*
			 * On these conditions child shares
			 * the parent of the caller
			 */
			list_add_tail(&task->child_ref,
				      &parent->parent->children);
			task->parent = parent->parent;
		} else {
			list_add_tail(&task->child_ref,
				      &parent->children);
			task->parent = parent;
		}
	} else {
		struct tcb *pager = find_task(PAGER_TID);

		/* All parentless tasks are children of the pager */
		list_add_tail(&task->child_ref, &pager->children);
		task->parent = pager;
	}

	return task;
}

/*
 * Copy argument and environment strings into task's stack in a
 * format that is expected by the C runtime.
 *
 * e.g. uclibc expects stack state:
 *
 * (low) |->argc|argv[0]|argv[1]|...|argv[argc] = 0|envp[0]|...|NULL| (high)
 *
 */
int task_args_to_user(char *user_stack, struct args_struct *args,
		      struct args_struct *env)
{
	BUG_ON((unsigned long)user_stack & 7);

	/* Copy argc */
	*((int *)user_stack) = args->argc;
	user_stack += sizeof(int);

	/* Copy argument strings one by one */
	for (int i = 0; i < args->argc; i++) {
		strcpy(user_stack, args->argv[i]);
		user_stack += strlen(args->argv[i]) + 1;
	}
	/* Put the null terminator integer */
	*((int *)user_stack) = 0;
	user_stack = user_stack + sizeof(int);

	/* Copy environment strings one by one */
	for (int i = 0; i < env->argc; i++) {
		strcpy(user_stack, env->argv[i]);
		user_stack += strlen(env->argv[i]) + 1;
	}

	return 0;
}

int task_map_stack(struct vm_file *f, struct exec_file_desc *efd, struct tcb *task,
		   struct args_struct *args, struct args_struct *env)
{
	/* First set up task's stack markers */
	unsigned long stack_used = align_up(args->size + env->size, 8);
	unsigned long arg_pages = __pfn(page_align_up(stack_used));
	char *args_on_stack;
	void *mapped;

	task->stack_end = USER_AREA_END;
	task->stack_start = USER_AREA_END - DEFAULT_STACK_SIZE;
	task->args_end = task->stack_end;
	task->args_start = task->stack_end - stack_used;

	BUG_ON(stack_used > DEFAULT_STACK_SIZE);

	/*
	 * mmap task's stack as anonymous memory.
	 * TODO: Add VMA_GROWSDOWN here so the stack can expand.
	 */
	if (IS_ERR(mapped = do_mmap(0, 0, task, task->stack_start,
				    VM_READ | VM_WRITE |
				    VMA_PRIVATE | VMA_ANONYMOUS,
				    __pfn(task->stack_end -
					  task->stack_start)))) {
		printf("do_mmap: Mapping stack failed with %d.\n",
		       (int)mapped);
		return (int)mapped;
	}

	/* Map the stack's part that will contain args and environment */
	args_on_stack =
		pager_validate_map_user_range2(task,
					       (void *)task->args_start,
					       stack_used, VM_READ | VM_WRITE);

	/* Copy arguments and env */
	task_args_to_user(args_on_stack, args, env);

	/* Unmap task's those stack pages from pager */
	pager_unmap_pages(args_on_stack, arg_pages);

	return 0;
}

/*
 * If bss comes consecutively after the data section, prefault the
 * last page of the data section and zero out the bit that contains
 * the beginning of bss. If bss spans into more pages, then map those
 * pages as anonymous pages which are mapped by the devzero file.
 */
int task_map_bss(struct vm_file *f, struct exec_file_desc *efd, struct tcb *task)
{
	unsigned long bss_mmap_start;
	void *mapped;

	/*
	 * Test if bss starts right from the end of data,
	 * and not on a new page boundary.
	 */
	if ((task->data_end == task->bss_start) &&
	    !is_page_aligned(task->bss_start)) {
		unsigned long bss_size = task->bss_end - task->bss_start;
		struct page *last_data_page;
		void *pagebuf, *bss;

		/* Prefault the last data page */
		BUG_ON(prefault_page(task, task->data_end,
				     VM_READ | VM_WRITE) < 0);
		/* Get the page */
		last_data_page = task_virt_to_page(task, task->data_end);

		/* Map the page */
		pagebuf = l4_map_helper((void *)page_to_phys(last_data_page), 1);

		/* Find the bss offset */
		bss = (void *)((unsigned long)pagebuf |
			       (PAGE_MASK & task->bss_start));

		/*
		 * Zero out the part that is bss. This is minimum of either
		 * end of bss or until the end of page, whichever is met first.
		 */
		memset((void *)bss, 0, min(TILL_PAGE_ENDS(task->data_end),
		       (int)bss_size));

		/* Unmap the page */
		l4_unmap_helper(pagebuf, 1);

		/* Push bss mmap start to next page */
		bss_mmap_start = page_align_up(task->bss_start);
	} else	/* Otherwise bss mmap start is same as bss_start */
		bss_mmap_start = task->bss_start;

	/*
	 * Now if there are more pages covering bss,
	 * map those as anonymous zero pages
	 */
	if (task->bss_end > bss_mmap_start) {
		if (IS_ERR(mapped = do_mmap(0, 0, task, task->bss_start,
					    VM_READ | VM_WRITE |
					    VMA_PRIVATE | VMA_ANONYMOUS,
					    __pfn(page_align_up(task->bss_end) -
						  page_align(task->bss_start))))) {
			printf("do_mmap: Mapping environment failed with %d.\n",
			       (int)mapped);
			return (int)mapped;
		}
	}

	return 0;
}

int task_mmap_segments(struct tcb *task, struct vm_file *file, struct exec_file_desc *efd,
		       struct args_struct *args, struct args_struct *env)
{
	void *mapped;
	//struct vm_file *shm;
	int err;

	/* mmap task's text to task's address space. */
	if (IS_ERR(mapped = do_mmap(file, efd->text_offset, task, task->text_start,
				    VM_READ | VM_WRITE | VM_EXEC | VMA_PRIVATE,
				    __pfn(page_align_up(task->text_end) -
				    page_align(task->text_start))))) {
		printf("do_mmap: failed with %d.\n", (int)mapped);
		return (int)mapped;
	}

	/* mmap task's data to task's address space. */
	if (IS_ERR(mapped = do_mmap(file, efd->data_offset, task, task->data_start,
				    VM_READ | VM_WRITE | VMA_PRIVATE,
				    __pfn(page_align_up(task->data_end) -
				    page_align(task->data_start))))) {
		printf("do_mmap: failed with %d.\n", (int)mapped);
		return (int)mapped;
	}

	/* mmap task's bss as anonymous memory. */
	if ((err = task_map_bss(file, efd, task)) < 0) {
		printf("%s: Mapping bss has failed.\n",
		       __FUNCTION__);
		return err;
	}

	/* mmap task's stack, writing in the arguments and environment */
	if ((err = task_map_stack(file, efd, task, args, env)) < 0) {
		printf("%s: Mapping task's stack has failed.\n",
		       __FUNCTION__);
		return err;
	}

	/*
	 * Task already has recycled task's utcb. It will attach to it
	 * when it starts in userspace.
	 */
	//if (IS_ERR(shm = shm_new((key_t)task->utcb, __pfn(DEFAULT_UTCB_SIZE))))
	//	return (int)shm;

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
		if (!(pc = task->entry))
			pc = task->text_start;
	if (!pager)
		pager = self_tid();

	/* Set up the task's thread details, (pc, sp, pager etc.) */
	exregs_set_stack(&exregs, sp);
	exregs_set_pc(&exregs, pc);
	exregs_set_pager(&exregs, pager);

	if ((err = l4_exchange_registers(&exregs, task->tid)) < 0) {
		printf("l4_exchange_registers failed with %d.\n", err);
		return err;
	}

	return 0;
}

int task_start(struct tcb *task)
{
	int err;
	struct task_ids ids = {
		.tid = task->tid,
		.spid = task->spid,
		.tgid = task->tgid,
	};

	/* Start the thread */
	printf("Starting task with id %d, spid: %d\n", task->tid, task->spid);
	if ((err = l4_thread_control(THREAD_RUN, &ids)) < 0) {
		printf("l4_thread_control failed with %d\n", err);
		return err;
	}

	return 0;
}

/*
 * During its initialisation FS0 wants to learn how many boot tasks
 * are running, and their tids, which includes itself. This function
 * provides that information.
 */
int vfs_send_task_data(struct tcb *vfs)
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

