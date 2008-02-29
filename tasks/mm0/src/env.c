/*
 * This implements a per-process virtual private file
 * server to store environment variables.
 *
 * Using a per-process private file for the environment
 * gives the impression as if a file-backed env/arg area
 * is mapped on every process. By this means the env/arg
 * pages dont need special processing and are abstracted
 * away as files. Same idea can be applied to other
 * private regions of a process such as the stack, so
 * that debuggers can use file-based process inspection
 * methods.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <l4lib/types.h>
#include <l4/lib/list.h>
#include <l4/api/kip.h>
#include <l4/api/errno.h>
#include <kmalloc/kmalloc.h>
#include <vm_area.h>
#include <string.h>
#include <file.h>
#include <task.h>

struct envdata {
	struct list_head list;
	void *env_data;
	int env_size;
	int id;
};
LIST_HEAD(env_list);

/* Copies environment data into provided page. */
int task_env_pager_read_page(struct vm_file *f, unsigned long f_off_pfn,
			     void *dest_page)
{
	struct envdata *env;

	list_for_each_entry(env, &env_list, list)
		if (env->id == f->vnum)
			goto copyenv;

	printf("%s: No such env id: %d, to copy environment for.\n",
	       __TASKNAME__,  f->vnum);
	return -EINVAL;

copyenv:
	if (f_off_pfn != 0) {
		printf("%s: Environments currently have a single page.\n");
		return -EINVAL;
	}

	memset(dest_page, 0, PAGE_SIZE);
	BUG_ON(env->env_size > PAGE_SIZE);
	memcpy(dest_page, env->env_data, env->env_size);

	return 0;
}

/* Pager for environment files */
struct vm_pager task_env_pager = {
	.ops = {
		.read_page = task_env_pager_read_page,
		.write_page= 0,
	},
};

/*
 * For a task that is about to execute, this dynamically
 * generates its environment file, and environment data.
 */
int task_prepare_environment(struct tcb *t)
{
	struct envdata *env;

	/* Allocate a new vmfile for this task's environment */
	if (IS_ERR(t->env_file = vmfile_alloc_init()))
		return (int)t->env_file;

	/* Initialise and add it to global vmfile list */

	/*
	 * NOTE: Temporarily we can use tid as the vnum because
	 * this is the only per-task file.
	 */
	t->env_file->vnum = t->tid;
	t->env_file->length = PAGE_SIZE;
	t->env_file->pager = &task_env_pager;
	list_add(&t->env_file->list, &vm_file_list);

	/* Allocate, initialise and add per-task env data */
	BUG_ON(!(env = kzalloc(sizeof(struct envdata))));
	INIT_LIST_HEAD(&env->list);
	env->env_data = &t->utcb_address;
	env->env_size = sizeof(t->utcb_address);
	env->id = t->tid;
	list_add(&env->list, &env_list);

	return 0;
}

