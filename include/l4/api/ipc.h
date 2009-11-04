#ifndef __IPC_H__
#define __IPC_H__


#define L4_NILTHREAD		0xFFFFFFFF
#define L4_ANYTHREAD		0xFFFFFFFE

#define L4_IPC_TAG_MR_OFFSET		0

/* Pagefault */
#define L4_IPC_TAG_PFAULT		0

#if defined (__KERNEL__)

/*
 * ipc syscall uses an ipc_type variable and send/recv
 * details are embedded in this variable.
 */
enum IPC_TYPE {
	IPC_INVALID = 0,
	IPC_SEND = 1,
	IPC_RECV = 2,
	IPC_SENDRECV = 3,
};

/* These are for internally created ipc paths. */
int ipc_send(l4id_t to, unsigned int flags);
int ipc_sendrecv(l4id_t to, l4id_t from, unsigned int flags);

#endif

#endif /* __IPC_H__ */
