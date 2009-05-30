/*
 * Main initialisation code for the ARM kernel
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include <l4/lib/string.h>
#include <l4/lib/idpool.h>
#include <l4/generic/kmalloc.h>
#include <l4/generic/platform.h>
#include <l4/generic/physmem.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/space.h>
#include <l4/generic/tcb.h>
#include INC_ARCH(linker.h)
#include INC_ARCH(asm.h)
#include INC_ARCH(bootdesc.h)
#include INC_SUBARCH(mm.h)
#include INC_SUBARCH(mmu_ops.h)
#include INC_GLUE(memlayout.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(message.h)
#include INC_GLUE(syscall.h)
#include INC_PLAT(platform.h)
#include INC_PLAT(printascii.h)
#include INC_API(syscall.h)
#include INC_API(kip.h)
#include INC_API(mutex.h)

unsigned int kernel_mapping_end;

void init_locks(void)
{
}

struct address_space pager_space;

/* Maps the early memory regions needed to bootstrap the system */
void init_kernel_mappings(void)
{
	init_clear_ptab();

	/* Map kernel area to its virtual region */
	add_section_mapping_init(virt_to_phys(_start_text),
				 (unsigned int)_start_text, 1,
				 cacheable | bufferable);

	/* Map kernel one-to-one to its physical region */
	add_section_mapping_init(virt_to_phys(_start_text),
				 virt_to_phys(_start_text),
				 1, 0);

	/* Map page table to its virtual region */
	add_section_mapping_init(virt_to_phys(_start_kspace),
				 (unsigned int)_start_kspace,
				 1, 0);

	/* Clean current before first time access. */
	memset(current, 0, sizeof(struct ktcb));

	/*
	 * We are currently on the bootstack. End of bootstack would
	 * eventually become the ktcb of the first pager. We use a
	 * statically allocated address_space structure for the pager.
	 */
	current->space = &pager_space;

	/* Access physical address of pager_space to assign with initial pgd */
	((struct address_space *)virt_to_phys(current->space))->pgd = &kspace;
}

void print_sections(void)
{
	dprintk("_start_kernel: ",(unsigned int)_start_kernel);
	dprintk("_start_text: ",(unsigned int)_start_text);
	dprintk("_end_text: ", 	(unsigned int)_end_text);
	dprintk("_start_data: ", (unsigned int)_start_data);
	dprintk("_end_data: ", 	(unsigned int)_end_data);
	dprintk("_start_vectors: ",(unsigned int)_start_vectors);
	dprintk("arm_high_vector: ",(unsigned int)arm_high_vector);
	dprintk("_end_vectors: ",(unsigned int)_end_vectors);
	dprintk("_start_kip: ", (unsigned int) _start_kip);
	dprintk("_end_kip: ", (unsigned int) _end_kip);
	dprintk("_bootstack: ",	(unsigned int)_bootstack);
	dprintk("_end_kernel: ",	(unsigned int)_end_kernel);
	dprintk("_start_kspace: ", (unsigned int)_start_kspace);
	dprintk("_start_pmd: ", (unsigned int)_start_pmd);
	dprintk("_end_pmd: ", (unsigned int)_end_pmd);
	dprintk("_end_kspace: ", (unsigned int)_end_kspace);
	dprintk("_end: ", (unsigned int)_end);
}

/*
 * Enable virtual memory using kernel's pgd
 * and continue execution on virtual addresses.
 */
void start_vm()
{
	/*
	 * TTB must be 16K aligned. This is because first level tables are
	 * sized 16K.
	 */
	if ((unsigned int)&kspace & 0x3FFF)
		dprintk("kspace not properly aligned for ttb:",
			(u32)&kspace);
	memset((void *)&kspace, 0, sizeof(pgd_table_t));
	arm_set_ttb(virt_to_phys(&kspace));

	/*
	 * This sets all 16 domains to zero and  domain 0 to 1. The outcome
	 * is that page table access permissions are in effect for domain 0.
	 * All other domains have no access whatsoever.
	 */
	arm_set_domain(1);

	/* Enable everything before mmu permissions are in place */
	arm_enable_caches();
	arm_enable_wbuffer();

	/*
	 * Leave the past behind. Tlbs are invalidated, write buffer is drained.
	 * The whole of I + D caches are invalidated unconditionally. This is
	 * important to ensure that the cache is free of previously loaded
	 * values. Otherwise unpredictable data aborts may occur at arbitrary
	 * times, each time a load/store operation hits one of the invalid
	 * entries and those entries are cleaned to main memory.
	 */
	arm_invalidate_cache();
	arm_drain_writebuffer();
	arm_invalidate_tlb();
	arm_enable_mmu();

	/* Jump to virtual memory addresses */
	__asm__ __volatile__ (
		"add	sp, sp, %0	\n"	/* Update stack pointer */
		"add	fp, fp, %0	\n"	/* Update frame pointer */
		/* On the next instruction below, r0 gets
		 * current PC + KOFFSET + 2 instructions after itself. */
		"add	r0, pc, %0	\n"
		/* Special symbol that is extracted and included in the loader.
		 * Debuggers can break on it to load the virtual symbol table */
		".global break_virtual;\n"
		"break_virtual:\n"
		"mov	pc, r0		\n" /* (r0 has next instruction) */
		:
		: "r" (KERNEL_OFFSET)
		: "r0"
	);

	/* At this point, execution is on virtual addresses. */
	remove_section_mapping(virt_to_phys(_start_kernel));

	/*
	 * Restore link register (LR) for this function.
	 *
	 * NOTE: LR values are pushed onto the stack at each function call,
	 * which means the restored return values will be physical for all
	 * functions in the call stack except this function. So the caller
	 * of this function must never return but initiate scheduling etc.
	 */
	__asm__ __volatile__ (
		"add	%0, %0, %1	\n"
		"mov	pc, %0		\n"
		:: "r" (__builtin_return_address(0)), "r" (KERNEL_OFFSET)
	);
	while(1);
}

/* This calculates what address the kip field would have in userspace. */
#define KIP_USR_OFFSETOF(kip, field) ((void *)(((unsigned long)&kip.field - \
					(unsigned long)&kip) + USER_KIP_PAGE))

/* The kip is non-standard, using 0xBB to indicate mine for now ;-) */
void kip_init()
{
	struct utcb **utcb_ref;

	memset(&kip, 0, PAGE_SIZE);
	memcpy(&kip, "L4\230K", 4); /* Name field = l4uK */
	kip.api_version 	= 0xBB;
	kip.api_subversion 	= 1;
	kip.api_flags 		= 0; 		/* LE, 32-bit architecture */
	kip.kdesc.subid		= 0x1;
	kip.kdesc.id 		= 0xBB;
	kip.kdesc.gendate 	= (__YEAR__ << 9)|(__MONTH__ << 5)|(__DAY__);
	kip.kdesc.subsubver 	= 0x00000001; /* Consider as .00000001 */
	kip.kdesc.ver 		= 0;
	memcpy(&kip.kdesc.supplier, "BBB", 3);

	kip_init_syscalls();

	/* KIP + 0xFF0 is pointer to UTCB segment start address */
	utcb_ref = (struct utcb **)((unsigned long)&kip + UTCB_KIP_OFFSET);

	/* All thread utcbs are allocated starting from UTCB_AREA_START */
	*utcb_ref = (struct utcb *)UTCB_AREA_START;

	add_mapping(virt_to_phys(&kip), USER_KIP_PAGE, PAGE_SIZE,
		    MAP_USR_RO_FLAGS);
}


void vectors_init()
{
	unsigned int size = ((u32)_end_vectors - (u32)arm_high_vector);

	/* Map the vectors in high vector page */
	add_mapping(virt_to_phys(arm_high_vector),
		    ARM_HIGH_VECTOR, size, 0);
	arm_enable_high_vectors();

	/* Kernel memory trapping is enabled at this point. */
}

void abort()
{
	printk("Aborting on purpose to halt system.\n");
#if 0
	/* Prefetch abort */
	__asm__ __volatile__ (
		"mov	pc, #0x0\n"
		::
	);
#endif
	/* Data abort */
	__asm__ __volatile__ (
		"mov	r0, #0		\n"
		"ldr	r0, [r0]	\n"
		::
	);
}

void jump(struct ktcb *task)
{
	__asm__ __volatile__ (
		"mov	lr,	%0\n"	/* Load pointer to context area */
		"ldr	r0,	[lr]\n"	/* Load spsr value to r0 */
		"msr	spsr,	r0\n"	/* Set SPSR as ARM_MODE_USR */
		"ldmib	lr, {r0-r14}^\n" /* Load all USR registers */

		"nop		\n"	/* Spec says dont touch banked registers
					 * right after LDM {no-pc}^ for one instruction */
		"add	lr, lr, #64\n"	/* Manually move to PC location. */
		"ldr	lr,	[lr]\n"	/* Load the PC_USR to LR */
		"movs	pc,	lr\n"	/* Jump to userspace, also switching SPSR/CPSR */
		:
		: "r" (task)
	);
}

void switch_to_user(struct ktcb *task)
{
	arm_clean_invalidate_cache();
	arm_invalidate_tlb();
	arm_set_ttb(virt_to_phys(TASK_PGD(task)));
	arm_invalidate_tlb();
	jump(task);
}

/*
 * Initialize the pager in the system.
 *
 * The pager uses the bootstack as its ktcb, the initial kspace as its pgd,
 * (kernel pmds are shared among all tasks) and a statically allocated
 * pager_space struct for its space structure.
 */
void init_pager(char *name, struct task_ids *ids)
{
	struct svc_image *taskimg = 0;
	struct ktcb *task;
	int task_pages;

	BUG_ON(strcmp(name, __PAGERNAME__));
	task = current;

	tcb_init(task);

	/*
	 * Search the compile-time generated boot descriptor for
	 * information on available task images.
	 */
	for (int i = 0; i < bootdesc->total_images; i++) {
		if (!strcmp(name, bootdesc->images[i].name)) {
			taskimg = &bootdesc->images[i];
			break;
		}
	}
	BUG_ON(!taskimg);

	if (taskimg->phys_start & PAGE_MASK)
		printk("Warning, image start address not page aligned.\n");

	/* Calculate the number of pages the task sections occupy. */
	task_pages = __pfn((page_align_up(taskimg->phys_end) -
			    page_align(taskimg->phys_start)));
	task->context.pc = INITTASK_AREA_START;

	/* Stack starts one page above the end of image. */
	task->context.sp = INITTASK_AREA_END - 8;
	task->context.spsr = ARM_MODE_USR;

	set_task_ids(task, ids);

	/* Pager gets first UTCB area available by default */
	task->utcb_address = UTCB_AREA_START;

	BUG_ON(!TASK_PGD(task));

	/*
	 * This task's userspace mapping. This should allocate a new pmd, if not
	 * existing, and a new page entry on its private pgd.
	 */
	add_mapping_pgd(taskimg->phys_start, INITTASK_AREA_START,
			task_pages * PAGE_SIZE, MAP_USR_DEFAULT_FLAGS,
			TASK_PGD(task));
	//printk("Mapping %d pages from 0x%x to 0x%x for %s\n", task_pages,
	//       taskimg->phys_start, INITTASK_AREA_START, name);

	/* Add the physical pages used by the task to the page map */
	set_page_map(taskimg->phys_start, task_pages, 1);

	/* Task's rendezvous point */
	waitqueue_head_init(&task->wqh_send);
	waitqueue_head_init(&task->wqh_recv);
	waitqueue_head_init(&task->wqh_pager);

	/* Global hashlist that keeps all existing tasks */
	tcb_add(task);

	/* Scheduler initialises the very first task itself */
}

void init_tasks()
{
	struct task_ids ids;

	/* Initialise thread and space id pools */
	thread_id_pool = id_pool_new_init(THREAD_IDS_MAX);
	space_id_pool = id_pool_new_init(SPACE_IDS_MAX);
	ids.tid = id_new(thread_id_pool);
	ids.spid = id_new(space_id_pool);
	ids.tgid = ids.tid;

	/* Initialise the global task and address space lists */
	init_ktcb_list();
	init_address_space_list();
	init_mutex_queue_head();

	printk("%s: Initialized. Starting %s as pager.\n",
	       __KERNELNAME__, __PAGERNAME__);
	/*
	 * This must come last so that other tasks can copy its pgd before it
	 * modifies it for its own specifics.
	 */
	init_pager(__PAGERNAME__, &ids);
}

void start_kernel(void)
{
	printascii("\n"__KERNELNAME__": start kernel...\n");
	/* Print section boundaries for kernel image */
	//print_sections();

	/* Initialise section mappings for the kernel area */
	init_kernel_mappings();

	/* Enable virtual memory and jump to virtual addresses */
	start_vm();

	/* PMD tables initialised */
	init_pmd_tables();

	/* Initialise platform-specific page mappings, and peripherals */
	platform_init();

	printk("%s: Virtual memory enabled.\n", __KERNELNAME__);

	/* Map and enable high vector page. Faults can be handled after here. */
	vectors_init();

	/* Remap 1MB kernel sections as 4Kb pages. */
	remap_as_pages(_start_kernel, _end_kernel);

	/* Move the initial pgd into a more convenient place, mapped as pages. */
	relocate_page_tables();

	/* Initialise memory allocators */
	paging_init();

	/* Initialise kip and map for userspace access */
	kip_init();

	/* Initialise system call page */
	syscall_init();

	/* Initialise everything else, e.g. locks, lists... */
	init_locks();

	/* Setup inittask's ktcb and push it to scheduler runqueue */
	init_tasks();

	/* Start the scheduler with available tasks in the runqueue */
	scheduler_start();

	BUG();
}

