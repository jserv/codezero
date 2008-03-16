/*
 * vm objects.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <vm_area.h>
#include <l4/macros.h>
#include <l4/api/errno.h>
#include <kmalloc/kmalloc.h>


void print_cache_pages(struct vm_object *vmo)
{
	struct page *p;

	printf("Pages:\n======\n");
	list_for_each_entry(p, &vmo->page_cache, list) {
		printf("Page offset: 0x%x, virtual: 0x%x, refcnt: %d\n", p->offset,
		       p->virtual, p->refcnt);
	}
}

void vm_object_print(struct vm_object *vmo)
{
	struct vm_file *f;

	printf("Object type: %s %s. Refs: %d. Pages in cache: %d.\n",
	       vmo->flags & VM_WRITE ? "writeable" : "read-only",
	       vmo->flags & VM_OBJ_FILE ? "file" : "shadow", vmo->refcnt,
	       vmo->npages);
	if (vmo->flags & VM_OBJ_FILE) {
		f = vm_object_to_file(vmo);
		printf("File type: %s\n",
		       (f->type == VM_FILE_DEVZERO) ? "devzero" :
		       ((f->type == VM_FILE_BOOTFILE) ? "bootfile" : "regular"));
		// printf("File type: 0x%x\n", f->type);
	}
	print_cache_pages(vmo);
	printf("\n");
}

/* Global list of in-memory vm objects. */
LIST_HEAD(vm_object_list);

struct vm_object *vm_object_init(struct vm_object *obj)
{
	INIT_LIST_HEAD(&obj->list);
	INIT_LIST_HEAD(&obj->page_cache);

	return obj;
}

/* Allocate and initialise a vmfile, and return it */
struct vm_object *vm_object_create(void)
{
	struct vm_object *obj;

	if (!(obj = kzalloc(sizeof(*obj))))
		return 0;

	return vm_object_init(obj);
}

struct vm_file *vm_file_create(void)
{
	struct vm_file *f;

	if (!(f = kzalloc(sizeof(*f))))
		return PTR_ERR(-ENOMEM);

	INIT_LIST_HEAD(&f->list);
	vm_object_init(&f->vm_obj);
	f->vm_obj.flags = VM_OBJ_FILE;

	return f;
}

/* Deletes the object via its base, along with all its pages */
int vm_object_delete(struct vm_object *vmo)
{
	struct vm_file *f;

	/* Release all pages */
	vmo->pager->ops.release_pages(vmo);

	/* Remove from global list */
	list_del(&vmo->list);

	/* Check any references */
	BUG_ON(vmo->refcnt);
	BUG_ON(!list_empty(&vmo->shadowers));
	BUG_ON(!list_empty(&vmo->page_cache));

	/* Obtain and free via the base object */
	if (vmo->flags & VM_OBJ_FILE) {
		f = vm_object_to_file(vmo);
		kfree(f);
	} else if (vmo->flags & VM_OBJ_SHADOW)
		kfree(vmo);
	else BUG();

	return 0;
}

