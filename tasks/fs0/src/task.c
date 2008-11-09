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
#include <lib/idpool.h>
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

/* Allocate a vfs task structure according to given flags */
struct tcb *tcb_alloc_init(unsigned int flags)
{
	struct tcb *task;

	if (!(task = kzalloc(sizeof(struct tcb))))
		return PTR_ERR(-ENOMEM);

	/* Allocate new fs data struct if its not shared */
	if (!(flags & TCB_SHARED_FS)) {
		if (!(task->fs_data =
		      kzalloc(sizeof(*task->fs_data)))) {
			kfree(task);
			return PTR_ERR(-ENOMEM);
		}
		task->fs_data->tcb_refs = 1;
	}

	/* Allocate file structures if not shared */
	if (!(flags & TCB_SHARED_FILES)) {
		if (!(task->files =
		      kzalloc(sizeof(*task->files)))) {
			kfree(task->fs_data);
			kfree(task);
			return PTR_ERR(-ENOMEM);
		}
		if (IS_ERR(task->files->fdpool =
			   id_pool_new_init(TASK_FILES_MAX))) {
			void *err = task->files->fdpool;

			kfree(task->files);
			kfree(task->fs_data);
			kfree(task);
			return err;
		}
		task->files->tcb_refs = 1;
	}

	/* Ids will be set up later */
	task->tid = TASK_ID_INVALID;

	/* Initialise list structure */
	INIT_LIST_HEAD(&task->list);

	return task;
}

void copy_tcb(struct tcb *to, struct tcb *from, unsigned int share_flags)
{
	if (share_flags & TCB_SHARED_FILES) {
		to->files = from->files;
		to->files->tcb_refs++;
	} else {
		/* Copy all file descriptors */
		memcpy(to->files->fd, from->files->fd,
		       TASK_FILES_MAX * sizeof(to->files->fd[0]));

		/* Copy the idpool */
		id_pool_copy(to->files->fdpool, from->files->fdpool, TASK_FILES_MAX);
	}

	if (share_flags & TCB_SHARED_FS) {
		to->fs_data = from->fs_data;
		to->fs_data->tcb_refs++;
	} else
		memcpy(to->fs_data, from->fs_data, sizeof(*to->fs_data));
}

/* Allocate a task struct and initialise it */
struct tcb *tcb_create(struct tcb *orig, l4id_t tid, unsigned long utcb,
		       unsigned int share_flags)
{
	struct tcb *task;

	/* Can't have some share flags with no original task */
	BUG_ON(!orig && share_flags);

	/* Create a task and use given space and thread ids. */
	if (IS_ERR(task = tcb_alloc_init(share_flags)))
		return task;

	task->tid = tid;
	task->utcb_address = utcb;

	/*
	 * If there's an original task that means this will be a full
	 * or partial copy of it. We copy depending on share_flags.
	 */
	if (orig)
		copy_tcb(task, orig, share_flags);

	return task;
}

/* FIXME: Modify it to work with shared structures!!! */
void tcb_destroy(struct tcb *task)
{
	global_remove_task(task);

	kfree(task);
}

/*
 * Attaches to task's utcb. TODO: Add SHM_RDONLY and test it.
 * FIXME: This calls the pager and is a potential for deadlock
 * it only doesn't lock because it is called during initialisation.
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
int pager_notify_fork(struct tcb *sender, l4id_t parentid,
		      l4id_t childid, unsigned long utcb_address,
		      unsigned int flags)
{
	struct tcb *child, *parent;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);
	BUG_ON(!(parent = find_task(parentid)));

	/* Create a child vfs tcb using given parent and copy flags */
	if (IS_ERR(child = tcb_create(parent, childid, utcb_address, flags)))
		return (int)child;

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

	tcb_destroy(task);

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
		/* New tcb with fields sent by pager */
		if (IS_ERR(t = tcb_create(0, tdata_head->tdata[i].tid,
					  tdata_head->tdata[i].utcb_address,
					  0)))
			return (int)t;

		/* Initialise vfs specific fields. */
		t->fs_data->rootdir = vfs_root.pivot;
		t->fs_data->curdir = vfs_root.pivot;

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

