/*
 * Anonymous files for the process (e.g. stack, data, env)
 * are implemented here.
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
#include <proc.h>

static void *zpage_phys;
static struct page *zpage;
static struct vm_file *devzero;

/* TODO: Associate devzero with zero page */
void init_zero_page(void)
{
	void *zpage_virt;
	zpage_phys = alloc_page(1);
	zpage = phys_to_page(zpage_phys);

	/* Map it to self */
	zpage_virt = l4_map_helper(zpage_phys, 1);

	/* Zero it */
	memset(zpage_virt, 0, PAGE_SIZE);

	/* Unmap it */
	l4_unmap_helper(zpage_virt, 1);

	/* Update page struct. All other fields are zero */
	spin_lock(&page->lock);
	zpage->count++;
	spin_unlock(&page->lock);

}

#define VM_OBJ_MASK		0xFFFF
#define VM_OBJ_DEVZERO		(1 << 0)	/* Devzero special file */
#define VM_OBJ_FILE		(1 << 1)	/* Regular VFS file */
#define VM_OBJ_SHADOW		(1 << 2)	/* Shadow of another object */
#define VM_OBJ_COW		(1 << 3)	/* Copy-on-write semantics */

struct vm_object *get_devzero(void)
{
	return &devzero;
}

struct page *get_zero_page(void)
{

	/* Update zero page struct. */
	spin_lock(&page->lock);
	zpage->count++;
	spin_unlock(&page->lock);

	return zpage;
}

void put_zero_page(void)
{
	spin_lock(&page->lock);
	zpage->count--;
	spin_unlock(&page->lock);

	BUG_ON(zpage->count < 0);
}

#define vm_object_to_file(obj)	\
	(struct vm_file *)container_of(obj, struct vm_file, vm_obj)

/* Returns the page with given offset in this vm_object */
struct page *devzero_pager_page_in(struct vm_object *vm_obj,
				   unsigned long page_offset)
{
	struct vm_file *devzero = container_of(vm_obj, struct vm_file, vm_obj);
	struct page *zpage = devzero->priv_data;

	/* Update zero page struct. */
	spin_lock(&page->lock);
	zpage->count++;
	spin_unlock(&page->lock);

	return zpage;
}

struct vm_pager devzero_pager {
	.page_in = devzero_pager_page_in,
};

void init_devzero(void)
{
	init_zero_page();

	INIT_LIST_HEAD(&devzero.list);
	INIT_LIST_HEAD(&devzero.shadows);
	INIT_LIST_HEAD(&devzero.page_cache);

	/* Devzero has infinitely many pages ;-) */
	devzero.npages = -1;
	devzero.type = VM_OBJ_DEVZERO;
	devzero.pager = &devzero_pager;
}

/* Allocates and fills in the env page. This is like a pre-faulted file. */
int task_populate_env(struct task *task)
{
	void *paddr = alloc_page(1);
	void *vaddr = phys_to_virt(paddr);
	struct page *page = phys_to_page(paddr);

	/* Map new page at a self virtual address temporarily */
	l4_map(paddr, vaddr, 1, MAP_USR_RW_FLAGS, self_tid());

	/* Clear the page */
	memset((void *)vaddr, 0, PAGE_SIZE);

	/* Fill in environment data */
	memcpy((void *)vaddr, &t->utcb_address, sizeof(t->utcb_address));

	/* Remove temporary mapping */
	l4_unmap((void *)vaddr, 1, self_tid());

	spin_lock(&page->lock);

	/* Environment file owns this page */
	page->owner = task->proc_files->env_file;

	/* Add the page to it's owner's list of in-memory pages */
	BUG_ON(!list_empty(&page->list));
	insert_page_olist(page, page->owner);

	/* The offset of this page in its owner file */
	page->f_offset = 0;

	page->count++;
	page->virtual = 0;
	spin_unlock(&page->lock);

	return 0;
}

/*
 * For a task that is about to execute, this dynamically
 * generates its environment file, and environment data.
 */
int task_setup_vm_objects(struct tcb *t)
{
	struct proc_vm_objects *po = &t->proc_vm_objects;

	if (IS_ERR(pf->stack = vmfile_alloc_init()))
		return (int)t->stack_file;
	if (IS_ERR(pf->env = vmfile_alloc_init()))
		return (int)t->env_file;
	if (IS_ERR(pf->env_file = vmfile_alloc_init()))
		return (int)t->data_file;

	t->env_file->vnum = (t->tid << 16) | TASK_ENV_VNUM;
	t->env_file->length = t->env_end - t->env_start;
	t->env_file->pager = &task_anon_pager;
	list_add(&t->env_file->list, &vm_file_list);

	t->stack_file->vnum = (t->tid << 16) TASK_STACK_VNUM;
	t->stack_file->length = t->stack_end - t->stack_start;
	t->stack_file->pager = &task_anon_pager;
	list_add(&t->stack_file->list, &vm_file_list);

	t->data_file->vnum = (t->tid << 16) TASK_DATA_VNUM;
	t->data_file->length = t->data_end - t->data_start;
	t->data_file->pager = &task_anon_pager;
	list_add(&t->data_file->list, &vm_file_list);

	/* Allocate, initialise and add per-task env data */
	return task_populate_env(task);
}

