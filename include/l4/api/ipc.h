#ifndef __IPC_H__
#define __IPC_H__


#define L4_NILTHREAD		-1
#define L4_ANYTHREAD		-2

#define L4_IPC_TAG_MR_OFFSET		0


/* To synchronise two threads */
#define L4_IPC_TAG_WAIT			0

/* Pagefault */
#define L4_IPC_TAG_PFAULT		2

#if defined (__KERNEL__) /* These are kernel internal calls */
/* A helper call for sys_ipc() or internally created ipc paths. */
int ipc_send(l4id_t tid);

/*
 * This version sends an extra wait ipc to its receiver so that
 * the receiver can explicitly make it runnable later by accepting
 * this wait ipc.
 */
int ipc_sendwait(l4id_t tid);
#endif

#endif /* __IPC_H__ */
