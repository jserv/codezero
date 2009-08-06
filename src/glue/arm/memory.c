/*
 * ARM virtual memory implementation
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/lib/list.h>
#include <l4/lib/string.h>
#include <l4/lib/printk.h>
#include <l4/generic/physmem.h>
#include <l4/generic/space.h>
#include <l4/generic/tcb.h>
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memlayout.h)
#include INC_GLUE(memory.h)
#include INC_PLAT(printascii.h)
#include INC_PLAT(offsets.h)
#include INC_ARCH(linker.h)

/*
 * Conversion from generic protection flags to arch-specific
 * pte flags.
 */
unsigned int space_flags_to_ptflags(unsigned int flags)
{
	switch (flags) {
	case MAP_USR_RW_FLAGS:
		return __MAP_USR_RW_FLAGS;
	case MAP_USR_RO_FLAGS:
		return __MAP_USR_RO_FLAGS;
	case MAP_SVC_RW_FLAGS:
		return __MAP_SVC_RW_FLAGS;
	case MAP_USR_IO_FLAGS:
		return __MAP_USR_IO_FLAGS;
	case MAP_SVC_IO_FLAGS:
		return __MAP_SVC_IO_FLAGS;
	default:
		BUG();
	}
	BUG(); return 0;
}

void task_init_registers(struct ktcb *task, unsigned long pc)
{
	task->context.pc = (u32)pc;
	task->context.spsr = ARM_MODE_USR;
}

#if 0
/* Sets up struct page array and the physical memory descriptor. */
void paging_init(void)
{
	read_bootdesc();
	physmem_init();
	memory_init();
	copy_bootdesc();
}
#endif

/*
 * Copies global kernel entries into another pgd. Even for sub-pmd ranges
 * the associated pmd entries are copied, assuming any pmds copied are
 * applicable to all tasks in the system.
 */
void copy_pgd_kern_by_vrange(pgd_table_t *to, pgd_table_t *from,
			     unsigned long start, unsigned long end)
{
	/* Extend sub-pmd ranges to their respective pmd boundaries */
	start = align(start, PMD_MAP_SIZE);

	if (end < start)
		end = 0;

	/* Aligning would overflow if mapping the last virtual pmd */
	if (end < align(~0, PMD_MAP_SIZE) ||
	    start > end) /* end may have already overflown as input */
		end = align_up(end, PMD_MAP_SIZE);
	else
		end = 0;

	copy_pgds_by_vrange(to, from, start, end);
}

/* Copies all standard bits that a user process should have in its pgd */
void copy_pgd_kern_all(pgd_table_t *to)
{
	pgd_table_t *from = TASK_PGD(current);

	copy_pgd_kern_by_vrange(to, from, KERNEL_AREA_START, KERNEL_AREA_END);
	copy_pgd_kern_by_vrange(to, from, IO_AREA_START, IO_AREA_END);
	copy_pgd_kern_by_vrange(to, from, USER_KIP_PAGE,
				USER_KIP_PAGE + PAGE_SIZE);
	copy_pgd_kern_by_vrange(to, from, ARM_HIGH_VECTOR,
				ARM_HIGH_VECTOR + PAGE_SIZE);
	copy_pgd_kern_by_vrange(to, from, ARM_SYSCALL_VECTOR,
				ARM_SYSCALL_VECTOR + PAGE_SIZE);

	/* We temporarily map uart registers to every process */
	copy_pgd_kern_by_vrange(to, from, USERSPACE_UART_BASE,
				USERSPACE_UART_BASE + PAGE_SIZE);
}

