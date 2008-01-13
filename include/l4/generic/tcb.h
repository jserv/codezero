/*
 * Thread Control Block, kernel portion.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __TCB_H__
#define __TCB_H__

#include <l4/lib/list.h>
#include <l4/lib/mutex.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/pgalloc.h>
#include INC_GLUE(memory.h)
#include INC_GLUE(syscall.h)
#include INC_GLUE(utcb.h)
#include INC_SUBARCH(mm.h)

enum task_state {
	TASK_INACTIVE	= 0,
	TASK_SLEEPING	= 1,
	TASK_RUNNABLE	= 2,
};

/*
 * This describes the user space register context of each task. Simply set them
 * as regular structure fields, and they'll be copied onto real registers upon
 * a context switch. In the ARM case, they're copied from memory to userspace
 * registers using the LDM instruction with ^, no-pc flavor. See ARMARM.
 */
typedef struct arm_context {
	u32 spsr;	/* 0x0 */
	u32 r0;		/* 0x4 */
	u32 r1;		/* 0x8 */
	u32 r2;		/* 0xC */
	u32 r3;		/* 0x10 */
	u32 r4;		/* 0x14 */
	u32 r5;		/* 0x18 */
	u32 r6; 	/* 0x1C */
	u32 r7;		/* 0x20 */
	u32 r8;		/* 0x24 */
	u32 r9;		/* 0x28 */
	u32 r10;	/* 0x2C */
	u32 r11;	/* 0x30 */
	u32 r12;	/* 0x34 */
	u32 sp;		/* 0x38 */
	u32 lr;		/* 0x3C */
	u32 pc;		/* 0x40 */
} __attribute__((__packed__)) task_context_t;

#define TASK_ID_INVALID			-1
struct task_ids {
	l4id_t tid;
	l4id_t spid;
};

struct ktcb {
	/* User context */
	task_context_t context;

	/* Reference to syscall saved context */
	syscall_args_t *syscall_regs;

	/* Runqueue related */
	struct list_head rq_list;
	struct runqueue *rq;

	/* Thread information */
	l4id_t tid;		/* Global thread id */
	l4id_t spid;		/* Global space id */

	/* Flags to hint scheduler on future task state */
	unsigned int schedfl;
	unsigned int flags;

	/* Other related threads */
	l4id_t pagerid;

	u32 ts_need_resched;	/* Scheduling flag */
	enum task_state state;
	struct list_head task_list; /* Global task list. */
	struct utcb *utcb;	/* Reference to task's utcb area */

	/* Thread times */
	u32 kernel_time;	/* Ticks spent in kernel */
	u32 user_time;		/* Ticks spent in userland */
	u32 ticks_left;		/* Ticks left for reschedule */

	/* Page table information */
	pgd_table_t *pgd;

	/* Fields for ipc rendezvous */
	struct waitqueue_head wqh_recv;
	struct waitqueue_head wqh_send;

	/* Fields for checking parties blocked from doing ipc */
	struct spinlock ipc_block_lock;
	struct list_head ipc_block_list;

	l4id_t senderid;	/* Sender checks this for ipc */
};

/* Per thread kernel stack unified on a single page. */
union ktcb_union {
	struct ktcb ktcb;
	char kstack[PAGE_SIZE];
};

/* For traversing global task list */
extern struct list_head global_task_list;
static inline struct ktcb *find_task(l4id_t tid)
{
	struct ktcb *task;

	list_for_each_entry(task, &global_task_list, task_list)
		if (task->tid == tid)
			return task;
	return 0;
}

static inline int add_task_global(struct ktcb *new)
{
	INIT_LIST_HEAD(&new->task_list);
	list_add(&new->task_list, &global_task_list);
	return 0;
}


static inline void set_task_flags(struct ktcb *task, unsigned int fl)
{
	task->flags |= fl;
}

/*
 * Each task is allocated a unique global id. A thread group can only belong to
 * a single leader, and every thread can only belong to a single thread group.
 * These rules allow the fact that every global id can be used to define a
 * unique thread group id. Thread local ids are used as an index into the thread
 * group's utcb area to discover the per-thread utcb structure.
 */
static inline void set_task_ids(struct ktcb *task, struct task_ids *ids)
{
	task->tid = ids->tid;
	task->spid = ids->spid;
}

#define THREAD_IDS_MAX		1024
#define SPACE_IDS_MAX		1024


extern struct id_pool *thread_id_pool;
extern struct id_pool *space_id_pool;

#endif /* __TCB_H__ */

