/*
 * Userspace mutex implementation
 *
 * Copyright (C) 2009 Bahadir Bilgehan Balban
 */
#include <l4/lib/wait.h>
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/kmalloc.h>
#include <l4/generic/tcb.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include <l4/api/mutex.h>
#include INC_API(syscall.h)
#include INC_ARCH(exception.h)
#include INC_GLUE(memory.h)

struct mutex_queue {
	unsigned long physical;
	struct list_head list;
	struct waitqueue_head wqh;
};

struct mutex_queue_head {
	struct list_head list;
	int count;
} mutex_queue_head;

/*
 * Lock for mutex_queue create/deletion and also list add/removal.
 * Both operations are done jointly so a single lock is enough.
 */
struct mutex mutex_control_mutex;

void mutex_queue_head_lock()
{
	mutex_lock(&mutex_control_mutex);
}

void mutex_queue_head_unlock()
{
	mutex_unlock(&mutex_control_mutex);
}


void mutex_queue_init(struct mutex_queue *mq, unsigned long physical)
{
	/* This is the unique key that describes this mutex */
	mq->physical = physical;

	INIT_LIST_HEAD(&mq->list);
	waitqueue_head_init(&mq->wqh);
}

void mutex_control_add(struct mutex_queue *mq)
{
	BUG_ON(!list_empty(&mq->list));

	list_add(&mq->list, &mutex_queue_head.list);
	mutex_queue_head.count++;
}

void mutex_control_remove(struct mutex_queue *mq)
{
	list_del_init(&mq->list);
	mutex_queue_head.count--;
}

/* Note, this has ptr/negative error returns instead of ptr/zero. */
struct mutex_queue *mutex_control_find(unsigned long mutex_physical)
{
	struct mutex_queue *mutex_queue;

	/* Find the mutex queue with this key */
	list_for_each_entry(mutex_queue, &mutex_queue_head.list, list)
		if (mutex_queue->physical == mutex_physical)
			return mutex_queue;

	return 0;
}

struct mutex_queue *mutex_control_create(unsigned long mutex_physical)
{
	struct mutex_queue *mutex_queue;

	/* Allocate the mutex queue structure */
	if (!(mutex_queue = kzalloc(sizeof(struct mutex_queue))))
		return 0;

	/* Init and return */
	mutex_queue_init(mutex_queue, mutex_physical);

	return mutex_queue;
}

void mutex_control_delete(struct mutex_queue *mq)
{
	BUG_ON(!list_empty(&mq->list));

	/* Test internals of waitqueue */
	BUG_ON(&mq->wqh.sleepers);
	BUG_ON(!list_empty(&mq->wqh.task_list));

	kfree(mq);
}

/*
 * A contended thread is expected to show up with the
 * contended mutex address here.
 *
 * (1) The mutex is converted into its physical form and
 *     searched for in the existing mutex list. If it does not
 *     appear there, it gets added.
 * (2) The thread is put to sleep in the mutex wait queue
 *     until a wake up event occurs.
 */
int mutex_control_lock(unsigned long mutex_address)
{
	struct mutex_queue *mutex_queue;

	mutex_queue_head_lock();

	/* Search for the mutex queue */
	if (!(mutex_queue = mutex_control_find(mutex_address))) {
		/* Create a new one */
		if (!(mutex_queue = mutex_control_create(mutex_address))) {
			mutex_queue_head_unlock();
			return -ENOMEM;
		}
		/* Add the queue to mutex queue list */
		mutex_control_add(mutex_queue);
	}
	mutex_queue_head_unlock();

	/* Now sleep on the queue */
	wait_on(&mutex_queue->wqh);

	return 0;
}

/*
 * A thread that has detected a contention on a mutex that
 * it had locked but has just released is expected to show up with
 * that mutex here.
 *
 * (1) The mutex is converted into its physical form and
 *     searched for in the existing mutex list. If not found,
 *     the call returns an error.
 * (2) All the threads waiting on this mutex are woken up. This may
 *     cause a thundering herd, but user threads cannot be trusted
 *     to acquire the mutex, waking up all of them increases the
 *     chances that some thread may acquire it.
 */
int mutex_control_unlock(unsigned long mutex_address)
{
	struct mutex_queue *mutex_queue;

	mutex_queue_head_lock();

	/* Search for the mutex queue */
	if (!(mutex_queue = mutex_control_find(mutex_address))) {
		mutex_queue_head_unlock();
		/* No such mutex */
		return -ESRCH;
	}
	/* Found it, now wake all waiters up in FIFO order */
	wake_up_all(&mutex_queue->wqh, WAKEUP_ASYNC);

	/* Since noone is left, delete the mutex queue */
	mutex_control_remove(mutex_queue);
	mutex_control_delete(mutex_queue);

	/* Release lock and return */
	mutex_queue_head_unlock();
	return 0;
}

int sys_mutex_control(syscall_context_t *regs)
{
	unsigned long mutex_address = (unsigned long)regs->r0;
	int mutex_op = (int)regs->r1;
	unsigned long mutex_physical;
	int ret = 0;

	/* Check valid operation */
	if (mutex_op != MUTEX_CONTROL_LOCK &&
	    mutex_op != MUTEX_CONTROL_UNLOCK)
		return -EINVAL;

	/* Check valid user virtual address */
	if (!USER_ADDR(mutex_address))
		return -EINVAL;

	/* Find and check physical address for virtual mutex address */
	if (!(mutex_physical =
		virt_to_phys_by_pgd(mutex_address,
				   TASK_PGD(current))))
		return -EINVAL;

	switch (mutex_op) {
	case MUTEX_CONTROL_LOCK:
		ret = mutex_control_lock(mutex_physical);
		break;
	case MUTEX_CONTROL_UNLOCK:
		ret = mutex_control_unlock(mutex_physical);
		break;
	}

	return ret;
}


