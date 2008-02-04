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

enum IPC_TYPE {
	IPC_INVALID = 0,
	IPC_SEND = 1,
	IPC_RECV = 2,
	IPC_SENDRECV = 3
};

/*
 * Copies message registers from one ktcb stack to another. During the return
 * from system call, the registers are popped from the stack. On fast ipc path
 * they shouldn't even be pushed to the stack to avoid extra copying.
 */
int ipc_msg_copy(struct ktcb *to, struct ktcb *from)
{
	unsigned int *mr0_src = KTCB_REF_MR0(from);
	unsigned int *mr0_dst = KTCB_REF_MR0(to);

	/* NOTE:
	 * Make sure MR_TOTAL matches the number of registers saved on stack.
	 */
	memcpy(mr0_dst, mr0_src, MR_TOTAL * sizeof(unsigned int));

	return 0;
}

int sys_ipc_control(struct syscall_args *regs)
{
	return -ENOSYS;
}

int ipc_send(l4id_t recv_tid)
{
	struct ktcb *receiver = find_task(recv_tid);
	struct waitqueue_head *wqhs = &receiver->wqh_send;
	struct waitqueue_head *wqhr = &receiver->wqh_recv;

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
				//printk("(%d) Waking up (%d)\n", current->tid,
				//       sleeper->tid);

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
	//printk("(%d) waiting for (%d)\n", current->tid, recv_tid);
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
				//printk("(%d) Waking up (%d)\n", current->tid,
			       	//       sleeper->tid);

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
//	printk("(%d) waiting for (%d) \n", current->tid, current->senderid);
	spin_unlock(&wqhr->slock);
	spin_unlock(&wqhs->slock);
	return 0;
}

/* FIXME: REMOVE: remove this completely and replace by ipc_sendrecv() */
int ipc_sendwait(l4id_t to)
{
	unsigned int *mregs = KTCB_REF_MR0(current);

	/* Send actual message */
	ipc_send(to);

	/* Send wait message */
	mregs[L4_IPC_TAG_MR_OFFSET] = L4_IPC_TAG_WAIT;
	ipc_send(to);
	return 0;
}

/*
 * We currently only support send-receiving from the same task. The receive
 * stage is initiated with the special L4_IPC_TAG_IPCRETURN. This tag is used by
 * client tasks for receiving returned ipc results back. This is by far the most
 * common ipc pattern between client tasks and servers since every such ipc
 * request expects a result.
 */
int ipc_sendrecv(l4id_t to, l4id_t from)
{
	int ret = 0;

	if (to == from) {

		/* IPC send request stage */
		ipc_send(to);

		/*
		 * IPC result return stage.
		 *
		 * If the receiving task is scheduled here, (likely to be a
		 * server which shouldn't block too long) it would only block
		 * for a fixed amount of time between these send and receive
		 * calls.
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

/*
 * sys_ipc has multiple functions. In a nutshell:
 * - Copies message registers from one thread to another.
 * - Sends notification bits from one thread to another.
 * - Synchronises the threads involved in ipc. (i.e. a blocking rendez-vous)
 * - Can propagate messages from third party threads.
 * - A thread can both send and receive on the same call.
 */
int sys_ipc(struct syscall_args *regs)
{
	l4id_t to = (l4id_t)regs->r0;
	l4id_t from = (l4id_t)regs->r1;
	unsigned int ipc_type = 0;
	int ret = 0;

	/* Check arguments */
	if (!((from >= L4_ANYTHREAD) && (from <= MAX_PREDEFINED_TID))) {
		ret = -EINVAL;
		goto error;
	}
	if (!((to >= L4_ANYTHREAD) && (to <= MAX_PREDEFINED_TID))) {
		ret = -EINVAL;
		goto error;
	}
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
	printk("Erroneous ipc by: %d\n", current->tid);
	ipc_type = IPC_INVALID;
	return ret;
}

