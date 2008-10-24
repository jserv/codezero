/*
 * FS0 task data initialisation.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <l4/macros.h>
#include <l4/lib/list.h>
#include <l4/api/errno.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <lib/malloc.h>
#include <task.h>
#include <vfs.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <syscalls.h>
#include <globals.h>

struct global_list global_tasks = {
	.list = { &global_tasks.list, &global_tasks.list },
	.total = 0,
};

void global_add_task(struct tcb *task)
{
	BUG_ON(!list_empty(&task->list));
	list_add_tail(&task->list, &global_tasks.list);
	global_tasks.total++;
}
void global_remove_task(struct tcb *task)
{
	BUG_ON(list_empty(&task->list));
	list_del_init(&task->list);
	BUG_ON(--global_tasks.total < 0);
}


struct tcb *find_task(int tid)
{
	struct tcb *t;

	list_for_each_entry(t, &global_tasks.list, list)
		if (t->tid == tid)
			return t;
	return 0;
}

/* Allocate a task struct and initialise it */
struct tcb *create_tcb(void)
{
	struct tcb *t;

	if (!(t = kmalloc(sizeof(*t))))
		return PTR_ERR(-ENOMEM);

	t->fdpool = id_pool_new_init(TASK_FILES_MAX);
	INIT_LIST_HEAD(&t->list);

	return t;
}

void destroy_tcb(struct tcb *t)
{
	kfree(t->fdpool);

	global_remove_task(t);
	kfree(t);
}

/*
 * Attaches to task's utcb. FIXME: Add SHM_RDONLY and test it.
 * FIXME: This calls the pager and is a potential for deadlock
 */
int task_utcb_attach(struct tcb *t)
{
	int shmid;
	void *shmaddr;

	/* Use it as a key to create a shared memory region */
	if ((shmid = shmget((key_t)t->utcb_address, PAGE_SIZE, 0)) == -1)
		goto out_err;

	/* Attach to the region */
	if ((int)(shmaddr = shmat(shmid, (void *)t->utcb_address, 0)) == -1)
		goto out_err;

	/* Ensure address is right */
	if ((unsigned long)shmaddr != t->utcb_address)
		return -EINVAL;

	// printf("%s: Mapped utcb of task %d @ 0x%x\n",
	//       __TASKNAME__, t->tid, shmaddr);

	return 0;

out_err:
	printf("%s: Mapping utcb of task %d failed with err: %d.\n",
	       __TASKNAME__, t->tid, errno);
	return -EINVAL;
}

/*
 * Receives ipc from pager about a new fork event and
 * the information on the resulting child task.
 */
int pager_notify_fork(struct tcb *sender, l4id_t parid,
		      l4id_t chid, unsigned long utcb_address)
{
	struct tcb *child, *parent;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);
	BUG_ON(!(parent = find_task(parid)));

	if (IS_ERR(child = create_tcb()))
		return (int)child;

	/* Initialise fields sent by pager */
	child->tid = chid;
	child->utcb_address = utcb_address;

	/*
	 * Initialise vfs specific fields.
	 * FIXME: Or copy from parent???
	 */
	child->rootdir = vfs_root.pivot;
	child->curdir = vfs_root.pivot;

	/* Copy file descriptors from parent */
	id_pool_copy(child->fdpool, parent->fdpool, TASK_FILES_MAX);
	memcpy(child->fd, parent->fd, TASK_FILES_MAX * sizeof(int));

	global_add_task(child);

	// printf("%s/%s: Exiting...\n", __TASKNAME__, __FUNCTION__);
	return 0;
}


/*
 * Pager tells us that a task is exiting by this call.
 */
int pager_notify_exit(struct tcb *sender, l4id_t tid)
{
	struct tcb *task;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);
	BUG_ON(!(task = find_task(tid)));

	destroy_tcb(task);

	// printf("%s/%s: Exiting...\n", __TASKNAME__, __FUNCTION__);

	return 0;
}


/* Read task information into the utcb page, since it won't fit into mrs. */
struct task_data_head *receive_pager_taskdata(void)
{
	int err;

	/* Make the actual ipc call */
	if ((err = l4_sendrecv(PAGER_TID, PAGER_TID,
			       L4_IPC_TAG_TASKDATA)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return PTR_ERR(err);
	}

	/* Check if call itself was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: Error: %d.\n", __FUNCTION__, err);
		return PTR_ERR(err);
	}

	/* Data is expected in the utcb page */
	// printf("%s: %d Total tasks.\n", __FUNCTION__,
	//      ((struct task_data_head *)utcb_page)->total);

	return (struct task_data_head *)utcb_page;
}


int init_task_structs(struct task_data_head *tdata_head)
{
	struct tcb *t;

	for (int i = 0; i < tdata_head->total; i++) {
		if (IS_ERR(t = create_tcb()))
			return (int)t;

		/* Initialise fields sent by pager */
		t->tid = tdata_head->tdata[i].tid;
		t->utcb_address = tdata_head->tdata[i].utcb_address;

		/* Initialise vfs specific fields. */
		t->rootdir = vfs_root.pivot;
		t->curdir = vfs_root.pivot;

		/* Print task information */
		//printf("%s: Task info received from mm0:\n", __TASKNAME__);
		//printf("%s: task id: %d, utcb address: 0x%x\n",
		//       __TASKNAME__, t->tid, t->utcb_address);

		/* shm attach to the utcbs for all these tasks except own */
		if (t->tid != self_tid())
			task_utcb_attach(t);
		global_add_task(t);
	}

	return 0;
}

int init_task_data(void)
{
	struct task_data_head *tdata_head;

	/* Read how many tasks and tids of each */
	BUG_ON((tdata_head = receive_pager_taskdata()) < 0);

	/* Initialise local task structs using this info */
	BUG_ON(init_task_structs(tdata_head) < 0);

	return 0;
}

