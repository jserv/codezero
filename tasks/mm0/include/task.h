/*
 * Thread control block.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#ifndef __TASK_H__
#define __TASK_H__

#include <l4/macros.h>
#include <l4/types.h>
#include INC_GLUE(memlayout.h)
#include <l4/lib/list.h>
#include <l4lib/arch/types.h>
#include <l4lib/utcb.h>
#include <lib/addr.h>
#include <l4/api/kip.h>

#define __TASKNAME__		__PAGERNAME__

#define TASK_FILES_MAX			32

/* POSIX minimum is 4Kb */
#define DEFAULT_ENV_SIZE		SZ_4K
#define DEFAULT_STACK_SIZE		SZ_16K
#define DEFAULT_UTCB_SIZE		PAGE_SIZE


enum tcb_create_flags {
	TCB_NO_SHARING = 0,
	TCB_SHARED_VM = 1,
	TCB_SHARED_FILES = 2,
};

struct vm_file;

struct file_descriptor {
	unsigned long vnum;
	unsigned long cursor;
	struct vm_file *vmfile;
};

struct task_fd_head {
	struct file_descriptor fd[TASK_FILES_MAX];
	int tcb_refs;
};

struct task_vma_head {
	struct list_head list;
	int tcb_refs;
};


/* Stores all task information that can be kept in userspace. */
struct tcb {
	/* Task list */
	struct list_head list;

	/* Name of the task */
	char name[16];

	/* Task ids */
	int tid;
	int spid;
	int tgid;

	/* Related task ids */
	unsigned int pagerid;	/* Task's pager */

	/* Task's main address space region, usually USER_AREA_START/END */
	unsigned long start;
	unsigned long end;

	/* Page aligned program segment marks, ends exclusive as usual */
	unsigned long text_start;
	unsigned long text_end;
	unsigned long data_start;
	unsigned long data_end;
	unsigned long bss_start;
	unsigned long bss_end;
	unsigned long stack_start;
	unsigned long stack_end;
	unsigned long heap_start;
	unsigned long heap_end;
	unsigned long env_start;
	unsigned long env_end;
	unsigned long args_start;
	unsigned long args_end;

	/* Task's mmappable region */
	unsigned long map_start;
	unsigned long map_end;

	/* UTCB information */
	void *utcb;

	/* Virtual memory areas */
	struct task_vma_head *vm_area_head;

	/* File descriptors for this task */
	struct task_fd_head *files;
};

/* Structures to use when sending new task information to vfs */
struct task_data {
	unsigned long tid;
	unsigned long utcb_address;
};

struct task_data_head {
	unsigned long total;
	struct task_data tdata[];
};

struct tcb *find_task(int tid);
void task_add_global(struct tcb *t);

struct initdata;
void init_pm(struct initdata *initdata);

struct tcb *task_create(struct task_ids *ids,
			unsigned int ctrl_flags,
			unsigned int alloc_flags);
int send_task_data(l4id_t requester);
void task_map_prefault_utcb(struct tcb *mapper, struct tcb *owner);
int copy_tcb(struct tcb *to, struct tcb *from, unsigned int flags);

#endif /* __TASK_H__ */
