/*
 * Initialise the system.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/utcb.h>

#include <l4/lib/list.h>
#include <l4/generic/cap-types.h>	/* TODO: Move this to API */
#include <l4/api/capability.h>

#include <stdio.h>
#include <string.h>
#include <mm/alloc_page.h>
#include <lib/malloc.h>
#include <lib/bit.h>

#include <task.h>
#include <shm.h>
#include <file.h>
#include <init.h>
#include <test.h>
#include <boot.h>
#include <utcb.h>
#include <bootm.h>
#include <vfs.h>
#include <init.h>
#include <memory.h>
#include <capability.h>


extern unsigned long _start_init[];
extern unsigned long _end_init[];

/* Kernel data acquired during initialisation */
__initdata struct initdata initdata;


/* Physical memory descriptors */
struct memdesc physmem;		/* Initial, primitive memory descriptor */
struct membank membank[1];	/* The memory bank */
struct page *page_array;	/* The physical page array based on mem bank */


/* Capability descriptor list */
struct cap_list capability_list;

__initdata static struct capability *caparray;
__initdata static int total_caps = 0;


void print_pfn_range(int pfn, int size)
{
	unsigned int addr = pfn << PAGE_BITS;
	unsigned int end = (pfn + size) << PAGE_BITS;
	printf("Used: 0x%x - 0x%x\n", addr, end);
}

/*
 * A specialised function for setting up the task environment of mm0.
 * Essentially all the memory regions are set up but a new task isn't
 * created, registers aren't assigned, and thread not started, since
 * these are all already done by the kernel. But we do need a memory
 * environment for mm0, hence this function.
 */
int mm0_task_init(struct vm_file *f, unsigned long task_start,
		  unsigned long task_end, struct task_ids *ids)
{
	int err;
	struct tcb *task;

	/*
	 * The thread itself is already known by the kernel, so we just
	 * allocate a local task structure.
	 */
	BUG_ON(IS_ERR(task = tcb_alloc_init(TCB_NO_SHARING)));

	task->tid = ids->tid;
	task->spid = ids->spid;
	task->tgid = ids->tgid;

	/* Initialise vfs specific fields. */
	task->fs_data->rootdir = vfs_root.pivot;
	task->fs_data->curdir = vfs_root.pivot;

	if ((err = boottask_setup_regions(f, task,
					  task_start, task_end)) < 0)
		return err;

	if ((err =  boottask_mmap_regions(task, f)) < 0)
		return err;

	/* Set pager as child and parent of itself */
	list_insert(&task->child_ref, &task->children);
	task->parent = task;

	/*
	 * The first UTCB address is already assigned by the
	 * microkernel for this pager. Ensure that we also get
	 * the same from our internal utcb bookkeeping.
	 */
	BUG_ON(task->utcb_address != UTCB_AREA_START);

	/* Pager must prefault its utcb */
	prefault_page(task, task->utcb_address,
		      VM_READ | VM_WRITE);

	/* Add the task to the global task list */
	global_add_task(task);

	/* Add the file to global vm lists */
	global_add_vm_file(f);

	return 0;
}

/* Copy all init-memory allocated capabilities */
void copy_boot_capabilities()
{
	struct capability *cap;

	for (int i = 0; i < total_caps; i++) {
		cap = kzalloc(sizeof(struct capability));

		/* This copies kernel-allocated unique cap id as well */
		memcpy(cap, &caparray[i], sizeof(struct capability));

		/* Initialize capability list */
		link_init(&cap->list);

		/* Add capability to global cap list */
		list_insert(&capability_list.caps, &cap->list);
	}
}

int read_pager_capabilities()
{
	int ncaps;
	int err;

	/* Read number of capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_NCAPS, 0, &ncaps)) < 0) {
		printf("l4_capability_control() reading # of capabilities failed.\n"
		       "Could not complete CAP_CONTROL_NCAPS request.\n");
		goto error;
	}
	total_caps = ncaps;

	/* Allocate array of caps from boot memory */
	caparray = alloc_bootmem(sizeof(struct capability) * ncaps, 0);

	/* Read all capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_READ_CAPS, 0, caparray)) < 0) {
		printf("l4_capability_control() reading of capabilities failed.\n"
		       "Could not complete CAP_CONTROL_READ_CAPS request.\n");
		goto error;
	}

	/* Set up initdata pointer to important capabilities */
	initdata.bootcaps = caparray;
	for (int i = 0; i < ncaps; i++) {
		/*
		 * TODO: There may be multiple physmem caps!
		 * This really needs to be considered as multiple
		 * membanks!!!
		 */
		if ((caparray[i].type & CAP_RTYPE_MASK)
		    == CAP_RTYPE_PHYSMEM) {
			initdata.physmem = &caparray[i];
			return 0;
		}
	}
	printf("%s: Error, pager has no physmem capability defined.\n",
		__TASKNAME__);
	goto error;

	return 0;

error:
	BUG();
}

/*
 * Copy all necessary data from initmem to real memory,
 * release initdata and any init memory used
 */
void release_initdata()
{
	/*
	 * Copy boot capabilities to a list of
	 * real capabilities
	 */
	copy_boot_capabilities();

	/* Free and unmap init memory:
	 *
	 * FIXME: We can and do safely unmap the boot
	 * memory here, but because we don't utilize it yet,
	 * it remains as if it is a used block
	 */

	l4_unmap(_start_init,
		 __pfn(page_align_up(_end_init - _start_init)),
		 self_tid());
}

static void init_page_map(struct page_bitmap *pmap, unsigned long pfn_start, unsigned long pfn_end)
{
	pmap->pfn_start = pfn_start;
	pmap->pfn_end = pfn_end;
	set_page_map(pmap, pfn_start, pfn_end - pfn_start, 0);
}

/*
 * Marks pages in the global page_map as used or unused.
 *
 * @start = start page address to set, inclusive.
 * @numpages = number of pages to set.
 */
int set_page_map(struct page_bitmap *page_map, unsigned long pfn_start,
		 int numpages, int val)
{
	unsigned long pfn_end = pfn_start + numpages;
	unsigned long pfn_err = 0;

	if (page_map->pfn_start > pfn_start || page_map->pfn_end < pfn_start) {
		pfn_err = pfn_start;
		goto error;
	}
	if (page_map->pfn_end < pfn_end || page_map->pfn_start > pfn_end) {
		pfn_err = pfn_end;
		goto error;
	}

	if (val)
		for (int i = pfn_start; i < pfn_end; i++)
			page_map->map[BITWISE_GETWORD(i)] |= BITWISE_GETBIT(i);
	else
		for (int i = pfn_start; i < pfn_end; i++)
			page_map->map[BITWISE_GETWORD(i)] &= ~BITWISE_GETBIT(i);
	return 0;
error:
	BUG_MSG("Given page area is out of system page_map range: 0x%lx\n",
		pfn_err << PAGE_BITS);
	return -1;
}

/* Allocates page descriptors and initialises them using page_map information */
void init_physmem_secondary(struct membank *membank)
{
	struct page_bitmap *pmap = initdata.page_map;
	int npages = pmap->pfn_end - pmap->pfn_start;

	/* Allocation marks for the struct page array; npages, start, end */
	int pg_npages, pg_spfn, pg_epfn;
	unsigned long ffree_addr;

	/*
	 * Means the page array won't map one to one to pfns. That's ok,
	 * but we dont allow it for now.
	 */
	// BUG_ON(pmap->pfn_start);

	membank[0].start = __pfn_to_addr(pmap->pfn_start);
	membank[0].end = __pfn_to_addr(pmap->pfn_end);

	/* First find the first free page after last used page */
	for (int i = 0; i < npages; i++)
		if ((pmap->map[BITWISE_GETWORD(i)] & BITWISE_GETBIT(i)))
			membank[0].free = (i + 1) * PAGE_SIZE;
	BUG_ON(membank[0].free >= membank[0].end);

	/*
	 * One struct page for every physical page. Calculate how many pages
	 * needed for page structs, start and end pfn marks.
	 */
	pg_npages = __pfn(page_align_up((sizeof(struct page) * npages)));

	/* These are relative pfn offsets to the start of the memory bank */

	/* FIXME:
	 * 1.) These values were only right when membank started from pfn 0.
	 * 2.) Use set_page_map to set page map below instead of manually.
	 */
	pg_spfn = __pfn(membank[0].free);
	pg_epfn = pg_spfn + pg_npages;

	/* Use free pages from the bank as the space for struct page array */
	membank[0].page_array = l4_map_helper((void *)membank[0].free,
					      pg_npages);
	/* Update free memory left */
	membank[0].free += pg_npages * PAGE_SIZE;

	/* Update page bitmap for the pages used for the page array */
	for (int i = pg_spfn; i < pg_epfn; i++)
		pmap->map[BITWISE_GETWORD(i)] |= BITWISE_GETBIT(i);

	/* Initialise the page array */
	for (int i = 0; i < npages; i++) {
		link_init(&membank[0].page_array[i].list);

		/* Set use counts for pages the kernel has already used up */
		if (!(pmap->map[BITWISE_GETWORD(i)] & BITWISE_GETBIT(i)))
			membank[0].page_array[i].refcnt = -1;
		else	/* Last page used +1 is free */
			ffree_addr = (i + 1) * PAGE_SIZE;
	}

	/* First free address must come up the same for both */
	BUG_ON(ffree_addr != membank[0].free);

	/* Set global page array to this bank's array */
	page_array = membank[0].page_array;

	/* Test that page/phys macros work */
	BUG_ON(phys_to_page(page_to_phys(&page_array[5])) != &page_array[5])
}


/* Fills in the physmem structure with free physical memory information */
void init_physmem_primary()
{
	unsigned long pfn_start, pfn_end, pfn_images_end = 0;
	struct bootdesc *bootdesc = initdata.bootdesc;

	/* Allocate page map structure */
	initdata.page_map = alloc_bootmem(sizeof(struct page_bitmap) +
					  ((initdata.physmem->end -
					    initdata.physmem->start)
					    >> 5) + 1, 0);

	/* Initialise page map from physmem capability */
	init_page_map(initdata.page_map,
		      initdata.physmem->start,
		      initdata.physmem->end);

	/* Mark pager and other boot task areas as used */
	for (int i = 0; i < bootdesc->total_images; i++) {
		pfn_start =
			__pfn(page_align_up(bootdesc->images[i].phys_start));
		pfn_end = __pfn(page_align_up(bootdesc->images[i].phys_end));
		if (pfn_end > pfn_images_end)
			pfn_images_end = pfn_end;
		set_page_map(initdata.page_map, pfn_start,
			     pfn_end - pfn_start, 1);
	}

	physmem.start = initdata.physmem->start;
	physmem.end = initdata.physmem->end;

	physmem.free_cur = pfn_images_end;
	physmem.free_end = physmem.end;
	physmem.numpages = __pfn(physmem.end - physmem.start);
}

void init_physmem(void)
{
	init_physmem_primary();

	init_physmem_secondary(membank);

	init_page_allocator(membank[0].free, membank[0].end);
}

void init_execve(char *path)
{
	/*
	 * Find executable, and execute it by creating a new process
	 * rather than replacing the current image (which is the pager!)
	 */
}

/*
 * To be removed later: This file copies in-memory elf image to the
 * initialized and formatted in-memory memfs filesystem.
 */
void copy_init_process(void)
{
	//int fd = sys_open(find_task(self_tid()), "/test0", O_WRITE, 0)

	//sys_write(find_task(self_tid()), fd, __test0_start, __test0_end - __test0_start);

	//sys_close(find_task(self_tid()), fd);

}

void start_init_process(void)
{
	/* Copy raw test0 elf image from memory to memfs first */
	copy_init_process();

	init_execve("/test0");
}

void init(void)
{
	read_pager_capabilities();

	pager_address_pool_init();

	read_boot_params();

	init_physmem();

	init_devzero();

	shm_pool_init();

	utcb_pool_init();

	vfs_init();

	start_init_process();

	release_initdata();

	mm0_test_global_vm_integrity();
}

