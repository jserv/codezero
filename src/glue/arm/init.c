/*
 * Main initialisation code for the ARM kernel
 *
 * Copyright (C) 2007 - 2010 B Labs Ltd.
 */
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include <l4/lib/string.h>
#include <l4/lib/idpool.h>
#include <l4/generic/platform.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/space.h>
#include <l4/generic/tcb.h>
#include <l4/generic/idle.h>
#include <l4/generic/bootmem.h>
#include <l4/generic/resource.h>
#include <l4/generic/container.h>
#include INC_ARCH(linker.h)
#include INC_ARCH(asm.h)
#include INC_SUBARCH(mm.h)
#include INC_SUBARCH(cpu.h)
#include INC_SUBARCH(mmu_ops.h)
#include INC_SUBARCH(perfmon.h)
#include INC_GLUE(memlayout.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(mapping.h)
#include INC_GLUE(message.h)
#include INC_GLUE(syscall.h)
#include INC_GLUE(init.h)
#include INC_GLUE(smp.h)
#include INC_PLAT(platform.h)
#include INC_API(syscall.h)
#include INC_API(kip.h)
#include INC_API(mutex.h)

unsigned int kernel_mapping_end;

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
	dprintk("_start_syscalls: ", (unsigned int) _start_syscalls);
	dprintk("_end_syscalls: ", (unsigned int) _end_syscalls);
	dprintk("_start_bootstack: ", (unsigned int)_start_bootstack);
	dprintk("_end_bootstack: ", (unsigned int)_end_bootstack);
	dprintk("_start_bootstack: ", (unsigned int)_start_bootstack);
	dprintk("_end_bootstack: ", (unsigned int)_end_bootstack);
	dprintk("_start_init_pgd: ", (unsigned int)_start_init_pgd);
	dprintk("_end_init_pgd: ", (unsigned int)_end_init_pgd);
	dprintk("_end_kernel: ", (unsigned int)_end_kernel);
	dprintk("_start_init: ", (unsigned int)_start_init);
	dprintk("_end_init: ", (unsigned int)_end_init);
	dprintk("_end: ", (unsigned int)_end);
}

/* This calculates what address the kip field would have in userspace. */
#define KIP_USR_OFFSETOF(kip, field) ((void *)(((unsigned long)&kip.field - \
					(unsigned long)&kip) + USER_KIP_PAGE))

/* The kip is non-standard, using 0xBB to indicate mine for now ;-) */
void kip_init()
{
	/*
	 * TODO: Adding utcb size might be useful
	 */
	memset(&kip, 0, PAGE_SIZE);
	memcpy(&kip, "L4\230K", 4); /* Name field = l4uK */
	kip.api_version 	= 0xBB;
	kip.api_subversion 	= 1;
	kip.api_flags 		= 0; 		/* LE, 32-bit architecture */
	kip.kdesc.magic		= 0xBBB;
	kip.kdesc.version	= CODEZERO_VERSION;
	kip.kdesc.subversion	= CODEZERO_SUBVERSION;
	strncpy(kip.kdesc.date, __DATE__, KDESC_DATE_SIZE);
	strncpy(kip.kdesc.time, __TIME__, KDESC_TIME_SIZE);

	kip_init_syscalls();

	add_boot_mapping(virt_to_phys(&kip), USER_KIP_PAGE, PAGE_SIZE,
			 MAP_USR_RO);
	printk("%s: Kernel built on %s, %s\n", __KERNELNAME__,
	       kip.kdesc.date, kip.kdesc.time);
}

void vectors_init()
{
	unsigned int size = ((u32)_end_vectors - (u32)arm_high_vector);

	/* Map the vectors in high vector page */
	add_boot_mapping(virt_to_phys(arm_high_vector),
			 ARM_HIGH_VECTOR, size, MAP_KERN_RWX);

	/* Kernel memory trapping is enabled at this point. */
}


void remove_initial_mapping(void)
{
	/* At this point, execution is on virtual addresses. */
	remove_section_mapping(virt_to_phys(_start_kernel));
}

void init_finalize(void)
{
	platform_timer_start();

#if defined (CONFIG_SMP_)
	/* Tell other cores to continue */
	secondary_run_signal = 1;
	dmb();
#endif

	sched_resume_async(current);
	idle_task();
}

void start_kernel(void)
{
	print_early("\n"__KERNELNAME__": start kernel...\n");

	// print_sections();

	/* Early cpu initialization */
	cpu_startup();

	/*
	 * Initialise section mappings
	 * for the kernel area
	 */
	init_kernel_mappings();

	print_early("\n"__KERNELNAME__": Init kernel mappings...\n");

	/*
	 * Enable virtual memory
	 * and jump to virtual addresses
	 */
	start_virtual_memory();

	/*
	 * Set up initial page tables and ktcb
	 * as a valid environment for idle task
	 */
	setup_idle_task();

	/*
	 * Initialise platform-specific
	 * page mappings, and peripherals
	 */
	platform_init();

	/* Can only print when uart is mapped */
	printk("%s: Virtual memory enabled.\n",
	       __KERNELNAME__);

	/* Identify CPUs and system */
	system_identify();

	sched_init();

	/*
	 * Map and enable high vector page.
	 * Faults can be handled after here.
	 */
	vectors_init();

	/* Try to initialize secondary cores if there are any */
	smp_start_cores();

	/* Remove one-to-one kernel mapping */
	remove_initial_mapping();

	/* Remap 1MB kernel sections as 4Kb pages. */
	remap_as_pages((void *)page_align(_start_kernel),
		       (void *)page_align_up(_end_kernel));

	/*
	 * Initialise kip and map
	 * for userspace access
	 */
	kip_init();

	/* Initialise system call page */
	syscall_init();

	/* Init performance monitor, if enabled */
	perfmon_init();

	/*
	 * Evaluate system resources
	 * and set up resource pools
	 */
	init_system_resources(&kernel_resources);

	/*
	 * Free boot memory, switch to first
	 * task's stack and start scheduler
	 */
	init_finalize();

	BUG();
}

