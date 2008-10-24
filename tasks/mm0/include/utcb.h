#ifndef __MM0_UTCB_H__
#define __MM0_UTCB_H__

#include <l4lib/types.h>
#include <task.h>

#define UTCB_PREFAULT		(1 << 0)	/* Prefaults utcb pages after mapping */
#define	UTCB_NEW_SHM		(1 << 1)	/* Creates a new shm segment for utcb */
#define	UTCB_NEW_ADDRESS	(1 << 2)	/* Allocates a virtual address for utcb */

int utcb_pool_init(void);
void *utcb_new_address(void);
int utcb_delete_address(void *utcb_addr);

/* IPC to send utcb address information to tasks */
void *task_send_utcb_address(struct tcb *sender, l4id_t taskid);

int utcb_map_to_task(struct tcb *owner, struct tcb *mapper, unsigned int flags);
int utcb_unmap_from_task(struct tcb *owner, struct tcb *mapper);

/* Prefault an *mmaped* utcb */
int utcb_prefault(struct tcb *task, unsigned int vmflags);

#endif
