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


struct tcb_head {
	struct list_head list;
	int total;			/* Total threads */
} tcb_head;

struct tcb *find_task(int tid)
{
	struct tcb *t;

	list_for_each_entry(t, &tcb_head.list, list)
		if (t->tid == tid)
			return t;
	return 0;
}

/*
 * Asks pager to send information about currently running tasks. Since this is
 * called during initialisation, there can't be that many, so we assume message
 * registers are sufficient. First argument tells how many there are, the rest
 * tells the tids.
 */
int receive_pager_taskdata(l4id_t *tdata)
{
	int err;

	/* Make the actual ipc call */
	if ((err = l4_sendrecv(PAGER_TID, PAGER_TID,
			       L4_IPC_TAG_TASKDATA)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	/* Check if call itself was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: Error: %d.\n", __FUNCTION__, err);
		return err;
	}

	/* Read total number of tasks. Note already used one mr. */
	if ((tdata[0] = (l4id_t)read_mr(L4SYS_ARG0)) >= MR_UNUSED_TOTAL) {
		printf("%s: Error: Too many tasks to read. Won't fit in mrs.\n",
		       __FUNCTION__);
		BUG();
	}
	// printf("%s: %d Total tasks.\n", __FUNCTION__, tdata[0]);

	/* Now read task ids. */
	for (int i = 0; i < (int)tdata[0]; i++) {
		tdata[1 + i] = (l4id_t)read_mr(L4SYS_ARG1 + i);
		// printf("%s: Task id: %d\n", __FUNCTION__, tdata[1 + i]);
	}

	return 0;
}

/* Allocate a task struct and initialise it */
struct tcb *create_tcb(l4id_t tid)
{
	struct tcb *t;

	if (!(t = kmalloc(sizeof(*t))))
		return PTR_ERR(-ENOMEM);

	t->tid = tid;
	INIT_LIST_HEAD(&t->list);
	list_add_tail(&t->list, &tcb_head.list);
	tcb_head.total++;

	return t;
}

int init_task_structs(l4id_t *tdata)
{
	struct tcb *t;
	int total = tdata[0];

	for (int i = 0; i < total; i++) {
		if (IS_ERR(t = create_tcb(tdata[1 + i])))
			return (int)t;
		t->rootdir = vfs_root.pivot;
		t->curdir = vfs_root.pivot;
	}
	return 0;
}

int init_task_data(void)
{
	l4id_t tdata[MR_UNUSED_TOTAL];

	INIT_LIST_HEAD(&tcb_head.list);

	/* Read how many tasks and tids of each */
	BUG_ON(receive_pager_taskdata(tdata) < 0);

	/* Initialise local task structs using this info */
	BUG_ON(init_task_structs(tdata) < 0);

	return 0;
}

