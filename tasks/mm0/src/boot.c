
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

struct page *device_page_in(struct vm_object *vm_obj,
			    unsigned long page_offset)
{
	struct vm_file *dev_file;

	BUG_ON(vm_obj->type != VM_OBJ_DEVICE);
	dev_file = vm_object_to_file(vm_obj);

}

struct vm_pager device_pager {
	.page_in = device_page_in,
};
LIST_HEAD(&boot_files);

/* From bare boot images, create mappable device files */
int init_boot_files(struct initdata *initdata)
{
	int err;
	struct svc_image *img;
	unsigned int sp, pc;
	struct tcb *task;
	struct task_ids ids;
	struct bootdesc *bd;
	struct vm_file *boot_file;

	bd = initdata->bootdesc;

	for (int i = 0; i < bd->total_images; i++) {
		img = &bd->images[i];
		boot_file = kzalloc(sizeof(struct vm_file));
		INIT_LIST_HEAD(&boot_file->vm_obj.list);
		boot_file->priv_data = img;
		boot_file->length = img->phys_end - img->phys_start;
		boot_file->pager = &device_pager;
		list_add(&boot_file->vm_obj.list, &boot_files);
	}
}

