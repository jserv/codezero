/*
 * This is yet unused, it is more of an anticipation
 * of how mmaped devices would be mapped with a pager.
 */

struct mmap_device {
	struct list_head page_list;	/* Dyn-allocated page list */
	unsigned long pfn_start;	/* Physical pfn start */
	unsigned long pfn_end;		/* Physical pfn end */
};

struct page *mmap_device_page_in(struct vm_object *vm_obj,
				 unsigned long pfn_offset)
{
	struct vm_file *f = vm_obj_to_file(vm_obj);
	struct mmap_device *mmdev = f->private_data;
	struct page *page;

	/* Check if its within device boundary */
	if (pfn_offset >= mmdev->pfn_end - mmdev->pfn_start)
		return -1;

	/* Simply return the page if found */
	list_for_each_entry(page, &mmdev->page_list, list)
		if (page->offset == pfn_offset)
			return page;

	/* Otherwise allocate one of our own for that offset and return it */
	page = kzalloc(sizeof(struct page));
	INIT_LIST_HEAD(&page->list);
	spin_lock_init(&page->lock);
	page->offset = pfn_offset;
	page->owner = vm_obj;
	page->flags = DEVICE_PAGE;
	list_add(&page->list, &mmdev->page_list)

	return page;
}

/* All mmapable devices are handled by this */
struct vm_pager mmap_device_pager {
	.page_in = mmap_device_page_in,
};


