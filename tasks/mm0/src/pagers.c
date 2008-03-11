/*
 * Copyright (C) 2008 Bahadir Balban
 */
#include <l4/macros.h>
#include <l4/lib/list.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <mm/alloc_page.h>
#include <vm_area.h>
#include <string.h>
#include <file.h>
#include <init.h>
#include INC_ARCH(bootdesc.h)

struct page *find_page(struct vm_object *obj, unsigned long pfn)
{
	struct page *p;

	list_for_each_entry(p, &obj->page_cache, list)
		if (p->offset == pfn)
			return p;

	return 0;
}

int default_release_pages(struct vm_object *vm_obj)
{
	struct page *p;
	struct list_head *n;
	void *phys;

	list_for_each_entry_safe(p, n, &vm_obj->page_cache, list) {
		list_del(&p->list);
		BUG_ON(p->refcnt);

		/* Return page back to allocator */
		free_page(page_to_phys(p));

		/* Free the page structure */
		kfree(p);
	}
	return 0;
}

struct page *file_page_in(struct vm_object *vm_obj, unsigned long page_offset)
{
	struct vm_file *f = vm_object_to_file(vm_obj);
	struct page *page;
	void *vaddr, *paddr;
	int err;

	/* Check first if the file has such a page at all */
	if (__pfn(page_align_up(f->length) <= page_offset)) {
		printf("%s: %s: Trying to look up page %d, but file length "
		       "is %d bytes.\n", __TASKNAME__, __FUNCTION__,
		       page_offset, f->length);
		BUG();
	}

	/* The page is not resident in page cache. */
	if (!(page = find_page(vm_obj, page_offset))) {
		/* Allocate a new page */
		paddr = alloc_page(1);
		vaddr = phys_to_virt(paddr);
		page = phys_to_page(paddr);

		/* Map the page to vfs task */
		l4_map(paddr, vaddr, 1, MAP_USR_RW_FLAGS, VFS_TID);

		/* Syscall to vfs to read into the page. */
		if ((err = vfs_read(f->vnum, page_offset, 1, vaddr)) < 0)
			goto out_err;

		/* Unmap it from vfs */
		l4_unmap(vaddr, 1, VFS_TID);

		/* Update vm object details */
		vm_obj->npages++;

		/* Update page details */
		spin_lock(&page->lock);
		page->count++;
		page->owner = vm_obj;
		page->offset = page_offset;
		page->virtual = 0;

		/* Add the page to owner's list of in-memory pages */
		BUG_ON(!list_empty(&page->list));
		insert_page_olist(page, vm_obj);
		spin_unlock(&page->lock);
	}

	return page;

out_err:
	l4_unmap(vaddr, 1, VFS_TID);
	free_page(paddr);
	return PTR_ERR(err);
}

/*
 * This reads-in a range of pages from a file and populates the page cache
 * just like a page fault, but its not in the page fault path.
 */
int read_file_pages(struct vm_file *f, unsigned long pfn_start,
		    unsigned long pfn_end)
{
	struct page *p;

	for (int f_offset = pfn_start; f_offset < pfn_end; f_offset++)
		if (IS_ERR(p = f->vm_obj.pager->ops.page_in(&f->vm_obj,
							    f_offset)))
			return (int)p;

	return 0;
}

/*
 * All non-mmapable char devices are handled by this.
 * VFS calls those devices to read their pages
 */
struct vm_pager file_pager = {
	.ops = {
		.page_in = file_page_in,
		.release_pages = default_release_pages,
	},
};


/* A proposal for shadow vma container, could be part of vm_file->priv_data */
struct vm_swap_node {
	struct vm_file *swap_file;
	struct task_ids task_ids;
	struct address_pool *pool;
};

/*
 * This should save swap_node/page information either in the pte or in a global
 * list of swap descriptors, and then write the page into the possibly one and
 * only swap file.
 */
struct page *swap_page_in(struct vm_object *vm_obj, unsigned long file_offset)
{
	/* No swapping yet, so the page is either here or not here. */
	return find_page(vm_obj, page_offset);
}

struct vm_pager swap_pager = {
	.ops = {
		.page_in = swap_page_in,
		.release_pages = default_release_pages,
	},
};

/* Returns the page with given offset in this vm_object */
struct page *bootfile_page_in(struct vm_object *vm_obj,
			      unsigned long offset)
{
	struct vm_file *boot_file = vm_object_to_file(vm_obj);
	struct svc_image *img = boot_file->priv_data;
	struct page *page = phys_to_page(img->phys_start +
					 __pfn_to_addr(offset));

	spin_lock(&page->lock);
	page->count++;
	spin_unlock(&page->lock);

	/* FIXME: Why not add pages to linked list and update npages? */
	vm_obj->npages++;

	return page;
}

struct vm_pager bootfile_pager = {
	.ops = {
		.page_in = bootfile_page_in,
	},
};

LIST_HEAD(boot_file_list);

/* From bare boot images, create mappable device files */
int init_boot_files(struct initdata *initdata)
{
	struct bootdesc *bd = initdata->bootdesc;
	struct vm_file *boot_file;
	struct svc_image *img;

	INIT_LIST_HEAD(&initdata->boot_file_list);

	for (int i = 0; i < bd->total_images; i++) {
		img = &bd->images[i];
		boot_file = vm_file_alloc_init();
		boot_file->priv_data = img;
		boot_file->length = img->phys_end - img->phys_start;
		boot_file->type = VM_FILE_BOOTFILE;

		/* Initialise the vm object */
		boot_file->vm_obj.type = VM_OBJ_FILE;
		boot_file->vm_obj.pager = &bootfile_pager;

		/* Add the object to global vm_object list */
		list_add(&boot_file->vm_obj.list, &vm_object_list);

		/* Add the file to initdata's bootfile list */
		list_add(&boot_file->list, &initdata->boot_file_list);
	}
	return 0;
}

/* Returns the page with given offset in this vm_object */
struct page *devzero_page_in(struct vm_object *vm_obj,
			     unsigned long page_offset)
{
	struct vm_file *devzero = vm_object_to_file(vm_obj);
	struct page *zpage = devzero->priv_data;

	BUG_ON(!(devzero->type & VM_FILE_DEVZERO));

	/* Update zero page struct. */
	spin_lock(&zpage->lock);
	BUG_ON(zpage->count < 0);
	zpage->count++;
	spin_unlock(&zpage->lock);

	return zpage;
}

struct vm_pager devzero_pager = {
	.ops = {
		.page_in = devzero_page_in,
	},
};

struct vm_file *get_devzero(void)
{
	struct vm_file *f;

	list_for_each_entry(f, &vm_file_list, list)
		if (f->type & VM_FILE_DEVZERO)
			return f;
	return 0;
}

int init_devzero(void)
{
	void *zphys, *zvirt;
	struct page *zpage;
	struct vm_file *devzero;

	/* Allocate and initialise the zero page */
	zphys = alloc_page(1);
	zpage = phys_to_page(zphys);
	zvirt = l4_map_helper(zphys, 1);
	memset(zvirt, 0, PAGE_SIZE);
	l4_unmap_helper(zvirt, 1);
	zpage->count++;

	/* Allocate and initialise devzero file */
	devzero = vmfile_alloc_init();
	devzero->vm_obj.npages = ~0;
	devzero->vm_obj.pager = &devzero_pager;
	devzero->vm_obj.type = VM_OBJ_FILE;
	devzero->type = VM_FILE_DEVZERO;
	devzero->priv_data = zpage;

	list_add(&devzero->vm_obj.list, &vm_object_list);
	list_add(&devzero->list, &vm_file_list);
	return 0;
}

