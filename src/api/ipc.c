/*
 * Inter-process communication
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */
#include <l4/generic/tcb.h>
#include <l4/lib/mutex.h>
#include <l4/api/ipc.h>
#include <l4/api/thread.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include <l4/lib/bit.h>
#include <l4/lib/math.h>
#include <l4/generic/kmalloc.h>
#include INC_API(syscall.h)
#include INC_GLUE(message.h)
#include INC_GLUE(ipc.h)

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

int ipc_short_copy(struct ktcb *to, struct ktcb *from)
{
	unsigned int *mr0_src = KTCB_REF_MR0(from);
	unsigned int *mr0_dst = KTCB_REF_MR0(to);

	/* NOTE:
	 * Make sure MR_TOTAL matches the number of registers saved on stack.
	 */
	memcpy(mr0_dst, mr0_src, MR_TOTAL * sizeof(unsigned int));

	return 0;
}


/* Copy full utcb region from one task to another. */
int ipc_full_copy(struct ktcb *to, struct ktcb *from)
{
	struct utcb *from_utcb = (struct utcb *)from->utcb_address;
	struct utcb *to_utcb = (struct utcb *)to->utcb_address;
	int ret;

	/* First do the short copy of primary mrs */
	if ((ret = ipc_short_copy(to, from)) < 0)
		return ret;

	/* Check that utcb memory accesses won't fault us */
	if ((ret = tcb_check_and_lazy_map_utcb(to)) < 0)
		return ret;
	if ((ret = tcb_check_and_lazy_map_utcb(from)) < 0)
		return ret;

	/* Directly copy from one utcb to another */
	memcpy(to_utcb->mr_rest, from_utcb->mr_rest,
	       MR_REST * sizeof(unsigned int));

	return 0;
}
/*
 * Extended copy is asymmetric in that the copying always occurs from
 * the sender's kernel stack to receivers userspace buffers.
 */
int ipc_extended_copy(struct ktcb *to, struct ktcb *from)
{
	unsigned long size = min(from->extended_ipc_size,
				 to->extended_ipc_size);

	/*
	 * Copy from sender's kernel stack buffer
	 * to receiver's paged-in userspace buffer
	 */
	memcpy(to->extended_ipc_buffer,
	       from->extended_ipc_buffer, size);

	return 0;
}

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
	unsigned int recv_ipc_type;
	unsigned int send_ipc_type;
	unsigned int *mr0_dst;
	int ret = 0;

       	recv_ipc_type = tcb_get_ipc_type(to);
       	send_ipc_type = tcb_get_ipc_type(from);

	/*
	 * Check ipc type flags of both parties and
	 * use the following rules:
	 *
	 * SHORT	SHORT		-> SHORT IPC
	 * FULL		FULL/SHORT	-> FULL IPC
	 * EXTENDED	EXTENDED	-> EXTENDED IPC
	 * EXTENDED	NON-EXTENDED	-> ENOIPC
	 */

	switch(recv_ipc_type) {
	case IPC_FLAGS_SHORT:
		if (send_ipc_type == IPC_FLAGS_SHORT)
			ret = ipc_short_copy(to, from);
		if (send_ipc_type == IPC_FLAGS_FULL)
			ret = ipc_full_copy(to, from);
		if (send_ipc_type == IPC_FLAGS_EXTENDED)
			ret = -ENOIPC;
		break;
	case IPC_FLAGS_FULL:
		if (send_ipc_type == IPC_FLAGS_SHORT)
			ret = ipc_full_copy(to, from);
		if (send_ipc_type == IPC_FLAGS_FULL)
			ret = ipc_full_copy(to, from);
		if (send_ipc_type == IPC_FLAGS_EXTENDED)
			ret = -ENOIPC;
		break;
	case IPC_FLAGS_EXTENDED:
		if (send_ipc_type == IPC_FLAGS_EXTENDED)
			ret = ipc_extended_copy(to, from);
		if (send_ipc_type == IPC_FLAGS_SHORT)
			ret = -ENOIPC;
		if (send_ipc_type == IPC_FLAGS_FULL)
			ret = -ENOIPC;
		break;
	}

	/* Save the sender id in case of ANYTHREAD receiver */
	if (to->expected_sender == L4_ANYTHREAD) {
       		mr0_dst = KTCB_REF_MR0(to);
		mr0_dst[MR_SENDER] = from->tid;
	}

	return ret;
}

int sys_ipc_control(syscall_context_t *regs)
{
	return -ENOSYS;
}

/*
 * Upon an ipc error or exception, the sleeper task is
 * notified of it via flags set by this function.
 */
void ipc_signal_error(struct ktcb *sleeper, int retval)
{
	/*
	 * Only EFAULT and ENOIPC is expected for now
	 */
	BUG_ON(retval != -EFAULT && retval != -ENOIPC);

	/*
	 * Set ipc error flag for sleeper.
	 */
	if (retval == -EFAULT)
		sleeper->ipc_flags |= IPC_EFAULT;
	if (retval == -ENOIPC)
		sleeper->ipc_flags |= IPC_ENOIPC;
}

/*
 * After an ipc, if current task was the sleeping party,
 * this checks whether errors were signalled, clears
 * the ipc flags and returns the appropriate error code.
 */
int ipc_handle_errors(void)
{
	/* Did we wake up normally or get interrupted */
	if (current->flags & TASK_INTERRUPTED) {
		current->flags &= ~TASK_INTERRUPTED;
		return -EINTR;
	}

	/* Did ipc fail with a fault error? */
	if (current->ipc_flags & IPC_EFAULT) {
		current->ipc_flags &= ~IPC_EFAULT;
		return -EFAULT;
	}

	/* Did ipc fail with a general ipc error? */
	if (current->ipc_flags & IPC_ENOIPC) {
		current->ipc_flags &= ~IPC_ENOIPC;
		return -ENOIPC;
	}

	return 0;
}

/*
 * NOTE:
 * Why can we safely copy registers and resume task
 * after we release the locks? Because even if someone
 * tried to interrupt and wake up the other party, they
 * won't be able to, because the task's all hooks to its
 * waitqueue have been removed at that stage.
 */

/* Interruptible ipc */
int ipc_send(l4id_t recv_tid, unsigned int flags)
{
	struct ktcb *receiver = tcb_find(recv_tid);
	struct waitqueue_head *wqhs, *wqhr;
	int ret = 0;

	wqhs = &receiver->wqh_send;
	wqhr = &receiver->wqh_recv;

	spin_lock(&wqhs->slock);
	spin_lock(&wqhr->slock);

	/* Ready to receive and expecting us? */
	if (receiver->state == TASK_SLEEPING &&
	    receiver->waiting_on == wqhr &&
	    (receiver->expected_sender == current->tid ||
	     receiver->expected_sender == L4_ANYTHREAD)) {
		struct waitqueue *wq = receiver->wq;

		/* Remove from waitqueue */
		list_del_init(&wq->task_list);
		wqhr->sleepers--;
		task_unset_wqh(receiver);

		/* Release locks */
		spin_unlock(&wqhr->slock);
		spin_unlock(&wqhs->slock);

		/* Copy message registers */
		if ((ret = ipc_msg_copy(receiver, current)) < 0)
			ipc_signal_error(receiver, ret);

		// printk("%s: (%d) Waking up (%d)\n", __FUNCTION__,
		//       current->tid, receiver->tid);

		/* Wake it up, we can yield here. */
		sched_resume_sync(receiver);
		return ret;
	}

	/* The receiver is not ready and/or not expecting us */
	CREATE_WAITQUEUE_ON_STACK(wq, current);
	wqhs->sleepers++;
	list_add_tail(&wq.task_list, &wqhs->task_list);
	task_set_wqh(current, wqhs, &wq);
	sched_prepare_sleep();
	spin_unlock(&wqhr->slock);
	spin_unlock(&wqhs->slock);
	// printk("%s: (%d) waiting for (%d)\n", __FUNCTION__,
	//       current->tid, recv_tid);
	schedule();

	return ipc_handle_errors();
}

int ipc_recv(l4id_t senderid, unsigned int flags)
{
	struct waitqueue_head *wqhs, *wqhr;
	int ret = 0;

	wqhs = &current->wqh_send;
	wqhr = &current->wqh_recv;

	/*
	 * Indicate who we expect to receive from,
	 * so senders know.
	 */
	current->expected_sender = senderid;

	spin_lock(&wqhs->slock);
	spin_lock(&wqhr->slock);

	/* Are there senders? */
	if (wqhs->sleepers > 0) {
		struct waitqueue *wq, *n;
		struct ktcb *sleeper;

		BUG_ON(list_empty(&wqhs->task_list));

		/* Look for a sender we want to receive from */
		list_for_each_entry_safe(wq, n, &wqhs->task_list, task_list) {
			sleeper = wq->task;

			/* Found a sender that we wanted to receive from */
			if ((sleeper->tid == current->expected_sender) ||
			    (current->expected_sender == L4_ANYTHREAD)) {
				list_del_init(&wq->task_list);
				wqhs->sleepers--;
				task_unset_wqh(sleeper);
				spin_unlock(&wqhr->slock);
				spin_unlock(&wqhs->slock);

				/* Copy message registers */
				if ((ret = ipc_msg_copy(current, sleeper)) < 0)
					ipc_signal_error(sleeper, ret);

				// printk("%s: (%d) Waking up (%d)\n",
				// __FUNCTION__,
				//       current->tid, sleeper->tid);
				sched_resume_sync(sleeper);
				return ret;
			}
		}
	}

	/* The sender is not ready */
	CREATE_WAITQUEUE_ON_STACK(wq, current);
	wqhr->sleepers++;
	list_add_tail(&wq.task_list, &wqhr->task_list);
	task_set_wqh(current, wqhr, &wq);
	sched_prepare_sleep();
	// printk("%s: (%d) waiting for (%d)\n", __FUNCTION__,
	//       current->tid, current->expected_sender);
	spin_unlock(&wqhr->slock);
	spin_unlock(&wqhs->slock);
	schedule();

	return ipc_handle_errors();
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
int ipc_sendrecv(l4id_t to, l4id_t from, unsigned int flags)
{
	int ret = 0;

	if (to == from) {
		/* Send ipc request */
		if ((ret = ipc_send(to, flags)) < 0)
			return ret;

		/*
		 * Get reply.
		 * A client would block its server only very briefly
		 * between these calls.
		 */
		if ((ret = ipc_recv(from, flags)) < 0)
			return ret;
	} else {
		printk("%s: Unsupported ipc operation.\n", __FUNCTION__);
		ret = -ENOSYS;
	}
	return ret;
}

int ipc_sendrecv_extended(l4id_t to, l4id_t from, unsigned int flags)
{
	return 0;
}

/*
 * In extended receive, receive buffers are page faulted before engaging
 * in real ipc.
 */
int ipc_recv_extended(l4id_t sendertid, unsigned int flags)
{
	unsigned long msg_index;
	unsigned long ipc_address;
	unsigned int size;
	unsigned int *mr0_current;
	int err;

	/*
	 * Obtain primary message register index
	 * containing extended ipc buffer address
	 */
	msg_index = extended_ipc_msg_index(flags);

	/* Get the pointer to primary message registers */
       	mr0_current = KTCB_REF_MR0(current);

	/* Obtain extended ipc address */
	ipc_address = (unsigned long)mr0_current[msg_index];

	/* Obtain extended ipc size */
	size = extended_ipc_msg_size(flags);

	/* Check size is good */
	if (size > IPC_EXTENDED_MAX_SIZE)
		return -EINVAL;

	/* Set extended ipc copy size */
	current->extended_ipc_size = size;

	/* Engage in real ipc to copy to ktcb buffer */
	if ((err = ipc_recv(sendertid, flags)) < 0)
		return err;

	/*
	 * NOTE: After this point primary mrs may be trashed.
	 * We need to save them here if we want to retain their values
	 * after a page fault ipc. Currently they're not needed.
	 */

	/* Page fault user pages if needed */
	if ((err = check_access(ipc_address, size,
				MAP_USR_RW_FLAGS, 1)) < 0)
		return err;

	/*
	 * Now copy from ktcb to user buffers
	 */
	memcpy((void *)ipc_address,
	       current->extended_ipc_buffer,
	       current->extended_ipc_size);

	return 0;
}

/*
 * In extended IPC, userspace buffers are copied to process
 * kernel stack before engaging in real calls ipc. If page fault
 * occurs, only the current process time is consumed.
 */
int ipc_send_extended(l4id_t recv_tid, unsigned int flags)
{
	unsigned long msg_index;
	unsigned long ipc_address;
	unsigned int size;
	unsigned int *mr0_current;
	int err;

	/*
	 * Obtain primary message register index
	 * containing extended ipc buffer address
	 */
	msg_index = extended_ipc_msg_index(flags);

	/* Get the pointer to primary message registers */
       	mr0_current = KTCB_REF_MR0(current);

	/* Obtain extended ipc address */
	ipc_address = (unsigned long)mr0_current[msg_index];

	/* Obtain extended ipc size */
	size = extended_ipc_msg_size(flags);

	/* Check size is good */
	if (size > IPC_EXTENDED_MAX_SIZE)
		return -EINVAL;

	/* Set extended ipc copy size */
	current->extended_ipc_size = size;

	/*
	 * NOTE: After this point primary mrs may be trashed.
	 * We need to save them here if we want to retain their values
	 * after a page fault ipc. Currently they're not needed.
	 */

	/* Page fault those pages on the current task if needed */
	if ((err = check_access(ipc_address, size,
				MAP_USR_RW_FLAGS, 1)) < 0)
		return err;

	/*
	 * It is now safe to access user pages.
	 * Copy message from user buffer into current kernel stack
	 */
	memcpy(current->extended_ipc_buffer,
	       (void *)ipc_address, size);

	/* Now we can engage in the real ipc */
	return ipc_send(recv_tid, flags);
}

static inline int __sys_ipc(l4id_t to, l4id_t from,
			    unsigned int ipc_type, unsigned int flags)
{
	int ret;

	if (ipc_flags_get_type(flags) == IPC_FLAGS_EXTENDED) {
		switch (ipc_type) {
		case IPC_SEND:
			ret = ipc_send_extended(to, flags);
			break;
		case IPC_RECV:
			ret = ipc_recv_extended(from, flags);
			break;
		case IPC_SENDRECV:
			ret = ipc_sendrecv_extended(to, from, flags);
			break;
		case IPC_INVALID:
		default:
			printk("Unsupported ipc operation.\n");
			ret = -ENOSYS;
		}
	} else {
		switch (ipc_type) {
		case IPC_SEND:
			ret = ipc_send(to, flags);
			break;
		case IPC_RECV:
			ret = ipc_recv(from, flags);
			break;
		case IPC_SENDRECV:
			ret = ipc_sendrecv(to, from, flags);
			break;
		case IPC_INVALID:
		default:
			printk("Unsupported ipc operation.\n");
			ret = -ENOSYS;
		}
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
	unsigned int flags = (unsigned int)regs->r2;
	unsigned int ipc_type = 0;
	int ret = 0;

/*
	if ((flags & L4_IPC_FLAGS_TYPE_MASK) == L4_IPC_FLAGS_EXTENDED)
		__asm__ __volatile__ (
				"1:\n"
				"b 1b\n");
*/

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

	/* Encode ipc type in task flags */
	tcb_set_ipc_flags(current, flags);

	if ((ret = __sys_ipc(to, from, ipc_type, flags)) < 0)
		goto error;
	return ret;

error:
	/*
	 * This is not always an error. For example a send/recv
	 * thread may go to suspension before receive phase.
	 */
	//printk("Erroneous ipc by: %d. from: %d, to: %d, Err: %d\n",
	//	 current->tid, from, to, ret);
	ipc_type = IPC_INVALID;
	return ret;
}

