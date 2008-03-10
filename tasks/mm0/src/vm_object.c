/*
 * vm objects.
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

/* Deletes the object via its base, along with all its pages */
int vm_object_delete(struct vm_object *vmo)
{
	struct vm_file *f;

	/* Release all pages */
	vmo->pager.ops->release_pages(vmo);

	/* Remove from global list */
	list_del(&vmo->list);

	/* Check any references */
	BUG_ON(vmo->refcnt);
	BUG_ON(!list_empty(&vmo->shadowers));
	BUG_ON(!list_emtpy(&vmo->page_cache));

	/* Obtain and free via the base object */
	if (vmo->flags & VM_OBJ_FILE) {
		f = vm_object_to_file(vmo);
		kfree(f);
	} else if (vmo->flags & VM_OBJ_SHADOW)
		kfree(obj);
	else BUG();

	return 0;
}

