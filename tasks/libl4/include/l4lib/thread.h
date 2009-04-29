#ifndef __L4_THREAD_H__
#define __L4_THREAD_H__

#include <libl4/arch/utcb.h>
#include <libl4/arch/types.h>

struct l4_thread_struct {
	l4id_t tlid;			/* Thread local id */
	struct task_ids ids;		/* Thread L4-defined ids */
	struct utcb *utcb;		/* Thread utcb */
	unsigned long stack_start;	/* Thread start of stack */
};

#endif /* __L4_THREAD_H__ */
