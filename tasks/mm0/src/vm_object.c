/*
 * VM Objects.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <vm_area.h>
#include <l4/macros.h>
#include <l4/api/errno.h>
#include <kmalloc/kmalloc.h>


/* Global list of in-memory vm files. */
LIST_HEAD(vm_object_list);

/* Allocate and initialise a vmfile, and return it */
struct vm_object *vm_object_alloc_init(void)
{
	struct vm_object *obj;

	if (!(obj = kzalloc(sizeof(*obj))))
		return PTR_ERR(-ENOMEM);

	INIT_LIST_HEAD(&obj->list);
	INIT_LIST_HEAD(&obj->page_cache);
	INIT_LIST_HEAD(&obj->shadows);

	return obj;
}

