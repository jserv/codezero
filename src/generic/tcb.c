/*
 * Some ktcb related data
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include <l4/generic/space.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/preempt.h>
#include <l4/lib/idpool.h>
#include INC_ARCH(exception.h)

/* ID pools for threads and spaces. */
struct id_pool *thread_id_pool;
struct id_pool *space_id_pool;
struct id_pool *tgroup_id_pool;

/* Hash table for all existing tasks */
struct list_head global_task_list;

/* Offsets for ktcb fields that are accessed from assembler */
unsigned int need_resched_offset = offsetof(struct ktcb, ts_need_resched);
unsigned int syscall_regs_offset = offsetof(struct ktcb, syscall_regs);

