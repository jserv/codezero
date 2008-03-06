/*
 * Copyright (C) 2008 Bahadir Balban
 */
#include <vm_area.h>

struct page *file_page_in(struct vm_object *vm_obj, unsigned long page_offset)
{
	struct vm_file *f = vm_object_to_file(vm_obj);
	struct page *page;

	/* The page is not resident in page cache. */
	if (!(page = find_page(vm_obj, page_offset)))
		/* Allocate a new page */
		void *paddr = alloc_page(1);
		void *vaddr = phys_to_virt(paddr);
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
int read_file_pages(struct vm_file *vmfile, unsigned long pfn_start,
		    unsigned long pfn_end)
{
	struct page *page;

	for (int f_offset = pfn_start; f_offset < pfn_end; f_offset++)
		vmfile->vm_obj->pager.ops->page_in(vmfile->vm_obj, f_offset);

	return 0;
}

/*
 * All non-mmapable char devices are handled by this.
 * VFS calls those devices to read their pages
 */
struct vm_pager file_pager {
	.page_in = file_page_in,
};

/* Returns the page with given offset in this vm_object */
struct page *bootfile_page_in(struct vm_object *vm_obj,
			      unsigned long pfn_offset)
{
	struct vm_file *boot_file = vm_obj_to_file(vm_obj);
	struct svc_image *img = boot_file->priv_data;
	struct page *page = phys_to_page(img->phys_start +
					 __pfn_to_addr(pfn_offset));

	spin_lock(&page->lock);
	page->count++;
	spin_unlock(&page->lock);

	return page;
}

struct vm_pager bootfile_pager {
	.page_in = bootfile_page_in,
};

LIST_HEAD(&boot_file_list);

/* From bare boot images, create mappable device files */
int init_boot_files(struct initdata *initdata)
{
	struct svc_image *img;
	unsigned int sp, pc;
	struct tcb *task;
	struct task_ids ids;
	struct bootdesc *bd;
	struct vm_file *boot_file;
	int err;

	bd = initdata->bootdesc;
	INIT_LIST_HEAD(&initdata->boot_file_list);

	for (int i = 0; i < bd->total_images; i++) {
		img = &bd->images[i];
		boot_file = vm_file_alloc_init();
		boot_file->priv_data = img;
		boot_file->length = img->phys_end - img->phys_start;
		boot_file->pager = &bootfile_pager;
		boot_file->type = VM_FILE_BOOTFILE;

		/* Initialise the vm object */
		boot_file->vm_obj.type = VM_OBJ_FILE;

		/* Add the object to global vm_object list */
		list_add(&boot_file->vm_obj.list, &vm_object_list);

		/* Add the file to initdata's bootfile list */
		list_add(&boot_file->list, &initdata->boot_file_list);
	}
}

/* Returns the page with given offset in this vm_object */
struct page *devzero_page_in(struct vm_object *vm_obj,
			     unsigned long page_offset)
{
	struct vm_file *devzero = vm_obj_to_file(vm_obj);
	struct page *zpage = devzero->priv_data;

	/* Update zero page struct. */
	spin_lock(&page->lock);
	zpage->count++;
	spin_unlock(&page->lock);

	return zpage;
}

struct vm_pager devzero_pager {
	.page_in = devzero_page_in,
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
	devzero->vm_obj.pager = devzero_pager;
	devzero->vm_obj.type = VM_OBJ_FILE;
	devzero->type = VM_FILE_DEVZERO;
	devzero->priv_data = zpage;

	list_add(&devzero->vm_obj.list, &vm_object_list);
	list_add(&devzero->list, &vm_file_list);
	return 0;
}

