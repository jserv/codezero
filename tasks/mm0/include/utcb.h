#ifndef __MM0_UTCB_H__
#define __MM0_UTCB_H__

#include <l4lib/types.h>
#include <task.h>
void *utcb_vaddr_new(void);
int utcb_pool_init(void);
int utcb_vaddr_del(void *utcb_addr);


/* IPC to send utcb address information to tasks */
void *task_send_utcb_address(struct tcb *sender, l4id_t taskid);

/* Prefault an *mmaped* utcb */
int utcb_prefault(struct tcb *task, unsigned int vmflags);

#endif
