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
#include <l4lib/types.h>
#include <l4lib/utcb.h>
#include <lib/addr.h>

#define __TASKNAME__		__PAGERNAME__

#define TASK_FILES_MAX			32

/* POSIX minimum is 4Kb */
#define DEFAULT_ENV_SIZE		SZ_16K
#define DEFAULT_STACK_SIZE		SZ_16K
#define DEFAULT_UTCB_SIZE		PAGE_SIZE


struct vm_file;

struct file_descriptor {
	unsigned long vnum;
	unsigned long cursor;
	struct vm_file *vmfile;
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

	/* Related task ids */
	unsigned int pagerid;	/* Task's pager */

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

	/* UTCB address */
	unsigned long utcb_address;

	/* Virtual memory areas */
	struct list_head vm_area_list;

	/* File descriptors for this task */
	struct file_descriptor fd[TASK_FILES_MAX];
};

struct tcb *find_task(int tid);

struct initdata;
void init_pm(struct initdata *initdata);

void send_task_data(l4id_t requester);

#endif /* __TASK_H__ */
