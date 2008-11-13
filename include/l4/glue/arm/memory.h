/*
 * Includes memory-related architecture specific definitions and their
 * corresponding generic wrappers.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __GLUE_ARM_MEMORY_H__
#define __GLUE_ARM_MEMORY_H__

#include INC_ARCH(bootdesc.h) /* Definition of last loaded svc image address */
#include INC_GLUE(memlayout.h) /* Important generic definitions */
#include INC_SUBARCH(mm.h)

/* Generic definitions */
#define PAGE_SIZE			ARM_PAGE_SIZE
#define PAGE_MASK			ARM_PAGE_MASK
#define PAGE_BITS			ARM_PAGE_BITS

/*
 * This defines the largest size defined by this architecture that is
 * easily mappable. ARM supports 1MB mappings so it fits well. If it's
 * unsupported by the arch then a reasonable size could be 1MB.
 */
#define SECTION_SIZE			ARM_SECTION_SIZE
#define SECTION_MASK			ARM_SECTION_MASK
#define SECTION_BITS			ARM_SECTION_BITS

/* Aligns to the upper page (ceiling) FIXME: Must add a wraparound checker. */
#define page_align_up(addr) 		((((unsigned int)(addr)) + \
					  (PAGE_SIZE - 1)) & \
					 (~PAGE_MASK))
/* Aligns to the lower page (floor) */
#define page_align(addr)		(((unsigned int)(addr)) &  \
					 (~PAGE_MASK))

#define is_aligned(val, mask)		(!(((unsigned long)(val)) & mask))
#define is_page_aligned(val)		(!(((unsigned long)(val)) & PAGE_MASK))
#define page_boundary(x)		is_page_aligned(x)

/* Align to given size */
#define	align(addr, size)		(((unsigned int)(addr)) & (~(size-1)))

/* The bytes left until the end of the page that x is in */
#define TILL_PAGE_ENDS(x)	(PAGE_SIZE - ((unsigned long)(x) & PAGE_MASK))

/* Extract page frame number from address and vice versa. */
#define __pfn(x)		(((unsigned long)(x)) >> PAGE_BITS)
#define __pfn_to_addr(x)	(((unsigned long)(x)) << PAGE_BITS)

/* Extract physical address from page table entry (pte) */
#define __pte_to_addr(x)	(((unsigned long)(x)) & ~PAGE_MASK)

/* Minimum excess needed for word alignment */
#define SZ_WORD				sizeof(unsigned int)
#define WORD_BITS			32
#define WORD_BITS_LOG2			5
#define BITWISE_GETWORD(x)	((x) >> WORD_BITS_LOG2) /* Divide by 32 */
#define	BITWISE_GETBIT(x)	(1 << ((x) % WORD_BITS))

#define align_up(addr, size)		((((unsigned long)(addr)) + ((size) - 1)) & (~((size) - 1)))

/* Endianness conversion */
static inline void be32_to_cpu(unsigned int x)
{
	char *p = (char *)&x;
	char tmp;

	/* Swap bytes */
	tmp = p[0];
	p[0] = p[3];
	p[3] = tmp;

	tmp = p[1];
	p[1] = p[2];
	p[2] = tmp;
}

/* Some anticipated values in terms of memory consumption */
#define TASK_AVERAGE_SIZE		SZ_16MB
#define TASKS_PER_1MB_GRANT		28

extern pgd_table_t kspace;
extern pmd_table_t pmd_tables[];
extern unsigned long pmdtab_i;

void init_pmd_tables(void);
pmd_table_t *alloc_boot_pmd(void);

/*
 * Each time a pager grants memory to the kernel, these parameters are called
 * for in order to distribute the granted memory for different purposes.
 *
 * The granted memory is used in an architecture-specific way, e.g. for pgds,
 * pmds, and kernel stack. Therefore this should be defined per-arch.
 */
typedef struct grant_kmem_usage {
	unsigned long total_size;
	unsigned long total_tasks;
	unsigned long total_pgds;
	unsigned long total_pmds;
	unsigned long total_tcbs;
	unsigned long extra;
} grant_kmem_usage_t;

void paging_init(void);
void init_pmd_tables(void);
void init_clear_ptab(void);

unsigned int space_flags_to_ptflags(unsigned int flags);

void add_boot_mapping(unsigned int paddr, unsigned int vaddr,
		 unsigned int size, unsigned int flags);
void add_mapping_pgd(unsigned int paddr, unsigned int vaddr,
		     unsigned int size, unsigned int flags,
		     pgd_table_t *pgd);
void add_mapping(unsigned int paddr, unsigned int vaddr,
		 unsigned int size, unsigned int flags);
int remove_mapping(unsigned long vaddr);
int remove_mapping_pgd(unsigned long vaddr, pgd_table_t *pgd);
void prealloc_phys_pagedesc(void);

int check_mapping_pgd(unsigned long vaddr, unsigned long size,
		      unsigned int flags, pgd_table_t *pgd);

int check_mapping(unsigned long vaddr, unsigned long size,
		  unsigned int flags);

void copy_pgd_kern_all(pgd_table_t *);
pte_t virt_to_pte(unsigned long virtual);
pte_t virt_to_pte_from_pgd(unsigned long virtual, pgd_table_t *pgd);

#endif /* __GLUE_ARM_MEMORY_H__ */

