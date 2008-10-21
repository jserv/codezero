#ifndef __MM0_UTCB_H__
#define __MM0_UTCB_H__

#include <l4lib/types.h>
#include <task.h>
int utcb_pool_init(void);
void *utcb_new_address(void);
int utcb_delete_address(void *utcb_addr);


/* IPC to send utcb address information to tasks */
void *task_send_utcb_address(struct tcb *sender, l4id_t taskid);

/* Prefault an *mmaped* utcb */
int utcb_prefault(struct tcb *task, unsigned int vmflags);

#endif
