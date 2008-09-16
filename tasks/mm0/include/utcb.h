#ifndef __MM0_UTCB_H__
#define __MM0_UTCB_H__

#include <l4lib/types.h>
#include <task.h>
void *utcb_vaddr_new(void);
int utcb_pool_init(void);


/* IPC to send utcb address information to tasks */
void *task_send_utcb_address(l4id_t sender, l4id_t taskid);

/* Prefault an *mmaped* utcb */
int utcb_prefault(struct tcb *task, unsigned int vmflags);

#endif
