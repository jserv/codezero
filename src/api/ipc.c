/*
 * Inter-process communication
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/tcb.h>
#include <l4/lib/mutex.h>
#include <l4/api/ipc.h>
#include <l4/api/thread.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include <l4/lib/bit.h>
#include <l4/generic/kmalloc.h>
#include INC_API(syscall.h)
#include INC_GLUE(message.h)

enum IPC_TYPE {
	IPC_INVALID = 0,
	IPC_SEND = 1,
	IPC_RECV = 2,
	IPC_SENDRECV = 3
};

/*
 * Copies message registers from one ktcb stack to another. During the return
 * from system call, the registers are popped from the stack. In the future
 * this should be optimised so that they shouldn't even be pushed to the stack
 *
 * This also copies the sender into MR0 in case the receiver receives from
 * L4_ANYTHREAD. This is done for security since the receiver cannot trust
 * the sender info provided by the sender task.
 */
int ipc_msg_copy(struct ktcb *to, struct ktcb *from)
{
	unsigned int *mr0_src = KTCB_REF_MR0(from);
	unsigned int *mr0_dst = KTCB_REF_MR0(to);

	/* NOTE:
	 * Make sure MR_TOTAL matches the number of registers saved on stack.
	 */
	memcpy(mr0_dst, mr0_src, MR_TOTAL * sizeof(unsigned int));

	/* Save the sender id in case of ANYTHREAD receiver */
	if (to->senderid == L4_ANYTHREAD)
		mr0_dst[MR_SENDER] = from->tid;

	return 0;
}

int sys_ipc_control(syscall_context_t *regs)
{
	return -ENOSYS;
}

int ipc_send(l4id_t recv_tid)
{
	struct ktcb *receiver = find_task(recv_tid);
	struct waitqueue_head *wqhs, *wqhr;

	if (!receiver) {
		printk("%s: tid: %d, no such task.\n", __FUNCTION__,
		       recv_tid);
		return -EINVAL;
	}
	wqhs = &receiver->wqh_send;
	wqhr = &receiver->wqh_recv;

	spin_lock(&wqhs->slock);
	spin_lock(&wqhr->slock);

	/* Is my receiver waiting? */
	if (wqhr->sleepers > 0) {
		struct waitqueue *wq, *n;
		struct ktcb *sleeper;

		list_for_each_entry_safe(wq, n, &wqhr->task_list, task_list) {
			sleeper = wq->task;
			/* Found the receiver. Does it sleep for this sender? */
			BUG_ON(sleeper->tid != recv_tid);
			if ((sleeper->senderid == current->tid) ||
			    (sleeper->senderid == L4_ANYTHREAD)) {
				list_del_init(&wq->task_list);
				spin_unlock(&wqhr->slock);
				spin_unlock(&wqhs->slock);

				/* Do the work */
				ipc_msg_copy(sleeper, current);
				//printk("%s: (%d) Waking up (%d)\n", __FUNCTION__,
				//       current->tid, sleeper->tid);

				/* Wake it up, we can yield here. */
				sched_resume_task(sleeper);
				return 0;
			}
		}
	}
	/* Could not find a receiver that's waiting */
	DECLARE_WAITQUEUE(wq, current);
	wqhs->sleepers++;
	list_add_tail(&wq.task_list, &wqhs->task_list);
	sched_notify_sleep(current);
	need_resched = 1;
	// printk("%s: (%d) waiting for (%d)\n", __FUNCTION__, current->tid, recv_tid);
	spin_unlock(&wqhr->slock);
	spin_unlock(&wqhs->slock);
	return 0;
}

int ipc_recv(l4id_t senderid)
{
	struct waitqueue_head *wqhs = &current->wqh_send;
	struct waitqueue_head *wqhr = &current->wqh_recv;

	/* Specify who to receiver from, so senders know. */
	current->senderid = senderid;

	spin_lock(&wqhs->slock);
	spin_lock(&wqhr->slock);

	/* Is my sender waiting? */
	if (wqhs->sleepers > 0) {
		struct waitqueue *wq, *n;
		struct ktcb *sleeper;

		list_for_each_entry_safe(wq, n, &wqhs->task_list, task_list) {
			sleeper = wq->task;
			/* Found a sender */
			if ((sleeper->tid == current->senderid) ||
			    (current->senderid == L4_ANYTHREAD)) {
				list_del_init(&wq->task_list);
				spin_unlock(&wqhr->slock);
				spin_unlock(&wqhs->slock);

				/* Do the work */
				ipc_msg_copy(current, sleeper);
				// printk("%s: (%d) Waking up (%d)\n", __FUNCTION__,
				//       current->tid, sleeper->tid);

				/* Wake it up */
				sched_resume_task(sleeper);
				return 0;

			}
		}
	}
	/* Could not find a sender that's waiting */
	DECLARE_WAITQUEUE(wq, current);
	wqhr->sleepers++;
	list_add_tail(&wq.task_list, &wqhr->task_list);
	sched_notify_sleep(current);
	need_resched = 1;
	// printk("%s: (%d) waiting for (%d) \n", __FUNCTION__, current->tid, current->senderid);
	spin_unlock(&wqhr->slock);
	spin_unlock(&wqhs->slock);
	return 0;
}

/*
 * Both sends and receives mregs in the same call. This is mainly by user
 * tasks for client server communication with system servers.
 *
 * Timeline of client/server communication using ipc_sendrecv():
 *
 * (1) User task (client) calls ipc_sendrecv();
 * (2) System task (server) calls ipc_recv() with from == ANYTHREAD.
 * (3) Rendezvous occurs. Both tasks exchange mrs and leave rendezvous.
 * (4,5) User task, immediately calls ipc_recv(), expecting a reply from server.
 * (4,5) System task handles the request in userspace.
 * (6) System task calls ipc_send() sending the return result.
 * (7) Rendezvous occurs. Both tasks exchange mrs and leave rendezvous.
 */
int ipc_sendrecv(l4id_t to, l4id_t from)
{
	int ret = 0;

	if (to == from) {
		/* Send ipc request */
		ipc_send(to);

		/*
		 * Get reply.
		 * A client would block its server only very briefly
		 * between these calls.
		 */
		ipc_recv(from);
	} else {
		printk("%s: Unsupported ipc operation.\n", __FUNCTION__);
		ret = -ENOSYS;
	}
	return ret;
}

static inline int __sys_ipc(l4id_t to, l4id_t from, unsigned int ipc_type)
{
	int ret;

	switch (ipc_type) {
	case IPC_SEND:
		ret = ipc_send(to);
		break;
	case IPC_RECV:
		ret = ipc_recv(from);
		break;
	case IPC_SENDRECV:
		ret = ipc_sendrecv(to, from);
		break;
	case IPC_INVALID:
	default:
		printk("Unsupported ipc operation.\n");
		ret = -ENOSYS;
	}
	return ret;
}

void printk_sysregs(syscall_context_t *regs)
{
	printk("System call registers for tid: %d\n", current->tid);
	printk("R0: %x\n", regs->r0);
	printk("R1: %x\n", regs->r1);
	printk("R2: %x\n", regs->r2);
	printk("R3: %x\n", regs->r3);
	printk("R4: %x\n", regs->r4);
	printk("R5: %x\n", regs->r5);
	printk("R6: %x\n", regs->r6);
	printk("R7: %x\n", regs->r7);
	printk("R8: %x\n", regs->r8);
}

/*
 * sys_ipc has multiple functions. In a nutshell:
 * - Copies message registers from one thread to another.
 * - Sends notification bits from one thread to another.
 * - Synchronises the threads involved in ipc. (i.e. a blocking rendez-vous)
 * - Can propagate messages from third party threads.
 * - A thread can both send and receive on the same call.
 */
int sys_ipc(syscall_context_t *regs)
{
	l4id_t to = (l4id_t)regs->r0;
	l4id_t from = (l4id_t)regs->r1;
	unsigned int ipc_type = 0;
	int ret = 0;

	/* Check arguments */
	if (from < L4_ANYTHREAD) {
		ret = -EINVAL;
		goto error;
	}
	if (to < L4_ANYTHREAD) {
		ret = -EINVAL;
		goto error;
	}

	/* Cannot send to self, or receive from self */
	if (from == current->tid || to == current->tid) {
		ret = -EINVAL;
		goto error;
	}

	/* [0] for Send */
	ipc_type |= (to != L4_NILTHREAD);

	/* [1] for Receive, [1:0] for both */
	ipc_type |= ((from != L4_NILTHREAD) << 1);

	if (ipc_type == IPC_INVALID) {
		ret = -EINVAL;
		goto error;
	}

	if ((ret = __sys_ipc(to, from, ipc_type)) < 0)
		goto error;
	return ret;

error:
	printk("Erroneous ipc by: %d. Err: %d\n", current->tid, ret);
	ipc_type = IPC_INVALID;
	return ret;
}

