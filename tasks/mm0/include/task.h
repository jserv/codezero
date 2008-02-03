/*
 * Thread control block.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __TASK_H__
#define __TASK_H__

#include <l4/macros.h>
#include <l4/types.h>
#include INC_GLUE(memlayout.h)
#include <l4/lib/list.h>
#include <l4lib/types.h>
#include <l4lib/utcb.h>

#define __TASKNAME__		"mm0"

/* Allow per-task anonymous memory to grow as much as 1 MB for now. */
#define TASK_SWAPFILE_MAXSIZE				SZ_1MB

struct vm_file;

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

	/* Program segment marks */
	unsigned long text_start;
	unsigned long text_end;
	unsigned long data_start;
	unsigned long data_end;
	unsigned long bss_start;
	unsigned long bss_end;
	unsigned long stack_start;
	unsigned long stack_end;   /* Exclusive of last currently mapped page */
	unsigned long heap_start;
	unsigned long heap_end;    /* Exclusive of last currently mapped page */

	/* Virtual memory areas */
	struct list_head vm_area_list;

	/* Per-task swap file for now */
	struct vm_file *swap_file;

	/* Pool to generate swap file offsets for fileless anonymous regions */
	struct id_pool *swap_file_offset_pool;
};

struct tcb *find_task(int tid);

struct initdata;
void init_pm(struct initdata *initdata);
int start_init_tasks(struct initdata *initdata);
void dump_tasks(void);

void send_task_data(l4id_t requester);

/* Used by servers that have a reference to tcbs (e.g. a pager) */
#define current		((struct ktcb *)__L4_ARM_Utcb()->usr_handle)

#endif /* __TASK_H__ */
