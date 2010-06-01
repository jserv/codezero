/*
 * Thread Control Block, kernel portion.
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */
#ifndef __TCB_H__
#define __TCB_H__


/*
 * These are a mixture of flags that indicate the task is
 * in a transitional state that could include one or more
 * scheduling states.
 */
#define TASK_INTERRUPTED		(1 << 0)
#define TASK_SUSPENDING			(1 << 1)
#define TASK_RESUMING			(1 << 2)
#define TASK_PENDING_SIGNAL		(TASK_SUSPENDING)
#define TASK_REALTIME			(1 << 5)

/*
 * This is to indicate a task (either current or one of
 * its children) exit has occured and cleanup needs to be
 * called
 */
#define TASK_EXITED			(1 << 3)

/* Task states */
enum task_state {
	TASK_INACTIVE	= 0,
	TASK_SLEEPING	= 1,
	TASK_RUNNABLE	= 2,
};

#define TASK_CID_MASK			0xFF000000
#define TASK_ID_MASK			0x00FFFFFF
#define TASK_CID_SHIFT			24

/* Values that rather have special meaning instead of an id value */
#define TASK_ID_INVALID			0xFFFFFFFF

#endif /* __TCB_H__ */

