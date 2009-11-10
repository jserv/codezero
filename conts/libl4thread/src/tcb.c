#include <stdio.h>
#include <malloc/malloc.h>
#include <l4/api/errno.h>
#include <l4/api/thread.h>
#include <tcb.h>

/* Global task list. */
struct l4t_global_list l4t_global_tasks = {
	.list = { &l4t_global_tasks.list, &l4t_global_tasks.list },
	.total = 0,
};

/* Function definitions */
void l4t_global_add_task(struct l4t_tcb *task)
{
	BUG_ON(!list_empty(&task->list));
	list_insert_tail(&task->list, &l4t_global_tasks.list);
	l4t_global_tasks.total++;
}

void l4t_global_remove_task(struct l4t_tcb *task)
{
	BUG_ON(list_empty(&task->list));
	list_remove_init(&task->list);
	BUG_ON(--l4t_global_tasks.total < 0);
}

struct l4t_tcb *l4t_find_task(int tid)
{
	struct l4t_tcb *t;

	list_foreach_struct(t, &l4t_global_tasks.list, list)
		if (t->tid == tid)
			return t;
	return 0;
}

struct l4t_tcb *l4t_tcb_alloc_init(struct l4t_tcb *parent, unsigned int flags)
{
	struct l4t_tcb *task;

	if (!(task = kzalloc(sizeof(struct l4t_tcb))))
		return PTR_ERR(-ENOMEM);

	link_init(&task->list);

	if ((parent) && (flags & TC_SHARE_SPACE))
		task->utcb_head = parent->utcb_head;
	else {
		if (!(task->utcb_head = kzalloc(sizeof(struct utcb_head))))
			return PTR_ERR(-ENOMEM);
		link_init(&task->utcb_head->list);
	}

	return task;
}
