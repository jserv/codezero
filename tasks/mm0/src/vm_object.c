/*
 * VM Objects.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <vm_area.h>
#include <l4/macros.h>
#include <l4/api/errno.h>
#include <kmalloc/kmalloc.h>


/* Global list of in-memory vm objects. */
LIST_HEAD(vm_object_list);

/* Global list of in-memory vm files */
LIST_HEAD(vm_file_list);

struct vm_object *vm_object_init(struct vm_object *obj)
{
	INIT_LIST_HEAD(&obj->list);
	INIT_LIST_HEAD(&obj->page_cache);
	INIT_LIST_HEAD(&obj->shadows);

	return obj;
}

/* Allocate and initialise a vmfile, and return it */
struct vm_object *vm_object_alloc_init(void)
{
	struct vm_object *obj;

	if (!(obj = kzalloc(sizeof(*obj))))
		return 0;

	return vm_object_init(obj);
}

struct vm_file *vm_file_alloc_init(void)
{
	struct vm_file *f;

	if (!(f = kzalloc(sizeof(*f))))
		return PTR_ERR(-ENOMEM);

	INIT_LIST_HEAD(&f->file_list);
	vm_object_init(&f->vm_obj);

	return f;
}

