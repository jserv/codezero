/*
 * ARM v5-specific virtual memory details
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __V5_MM_H__
#define __V5_MM_H__

/* ARM specific definitions */
#define VIRT_MEM_START			0
#define VIRT_MEM_END			0xFFFFFFFF
#define ARM_SECTION_SIZE		SZ_1MB
#define ARM_SECTION_MASK		(ARM_SECTION_SIZE - 1)
#define ARM_SECTION_BITS		20
#define ARM_PAGE_SIZE			SZ_4K
#define ARM_PAGE_MASK			0xFFF
#define ARM_PAGE_BITS			12

#define	PGD_SIZE				SZ_4K * 4
#define PGD_ENTRY_TOTAL				SZ_4K
#define	PGD_TYPE_MASK				0x3
#define	PGD_COARSE_ALIGN_MASK			0xFFFFFC00
#define	PGD_SECTION_ALIGN_MASK			0xFFF00000
#define	PGD_FINE_ALIGN_MASK			0xFFFFF000
#define	PGD_TYPE_FAULT				0
#define	PGD_TYPE_COARSE				1
#define	PGD_TYPE_SECTION			2
#define	PGD_TYPE_FINE				3

#define PMD_TYPE_MASK				0x3
#define PMD_TYPE_FAULT				0
#define PMD_TYPE_LARGE				1
#define PMD_TYPE_SMALL				2
#define PMD_TYPE_TINY				3

/* Permission field offsets */
#define SECTION_AP0				10

#define PMD_SIZE				SZ_1K
#define PMD_ENTRY_TOTAL				256
#define PMD_MAP_SIZE				SZ_1MB

/* Type-checkable page table elements */
typedef u32 pgd_t;
typedef u32 pmd_t;
typedef u32 pte_t;

/* Page global directory made up of pgd_t entries */
typedef struct pgd_table {
	pgd_t entry[PGD_ENTRY_TOTAL];
} pgd_table_t;

/* Page middle directory made up of pmd_t entries */
typedef struct pmd_table {
	pmd_t entry[PMD_ENTRY_TOTAL];
} pmd_table_t;

/* Applies for both small and large pages */
#define PAGE_AP0				4
#define PAGE_AP1				6
#define	PAGE_AP2				8
#define PAGE_AP3				10

/* Permission values with rom and sys bits ignored */
#define SVC_RW_USR_NONE				1
#define SVC_RW_USR_RO				2
#define SVC_RW_USR_RW				3

#define PTE_PROT_MASK				(0xFF << 4)

#define CACHEABILITY				3
#define BUFFERABILITY				2
#define cacheable				(1 << CACHEABILITY)
#define bufferable				(1 << BUFFERABILITY)
#define uncacheable				0
#define unbufferable				0

/* Helper macros for common cases */
#define __MAP_USR_RW_FLAGS	(cacheable | bufferable | (SVC_RW_USR_RW << PAGE_AP0)		\
				| (SVC_RW_USR_RW << PAGE_AP1) | (SVC_RW_USR_RW << PAGE_AP2)	\
				| (SVC_RW_USR_RW << PAGE_AP3))
#define __MAP_USR_RO_FLAGS	(cacheable | bufferable | (SVC_RW_USR_RO << PAGE_AP0)		\
				| (SVC_RW_USR_RO << PAGE_AP1) | (SVC_RW_USR_RO << PAGE_AP2)	\
				| (SVC_RW_USR_RO << PAGE_AP3))
#define __MAP_SVC_RW_FLAGS	(cacheable | bufferable | (SVC_RW_USR_NONE << PAGE_AP0) 	\
				| (SVC_RW_USR_NONE << PAGE_AP1) | (SVC_RW_USR_NONE << PAGE_AP2)	\
				| (SVC_RW_USR_NONE << PAGE_AP3))
#define __MAP_SVC_IO_FLAGS	(uncacheable | unbufferable | (SVC_RW_USR_NONE << PAGE_AP0)	\
				| (SVC_RW_USR_NONE << PAGE_AP1) | (SVC_RW_USR_NONE << PAGE_AP2)	\
				| (SVC_RW_USR_NONE << PAGE_AP3))
#define __MAP_USR_IO_FLAGS	(uncacheable | unbufferable | (SVC_RW_USR_RW << PAGE_AP0)	\
				| (SVC_RW_USR_RW << PAGE_AP1) | (SVC_RW_USR_RW << PAGE_AP2)	\
				| (SVC_RW_USR_RW << PAGE_AP3))

/* Abort information */

/*FIXME: Carry all these definitions to an abort.h, Also carry all abort code to abort.c. Much neater!!! */

/* Abort type */
#define ARM_PABT		1
#define ARM_DABT		0
/* The kernel makes use of bit 8 (Always Zero) of FSR to define which type of abort */
#define set_abort_type(fsr, x)	{ fsr &= ~(1 << 8); fsr |= ((x & 1) << 8); }
#define ARM_FSR_MASK		0xF
#define is_prefetch_abort(fsr)	((fsr >> 8) & 0x1)
#define is_data_abort(fsr)	(!is_prefetch_abort(fsr))

/*
 * v5 Architecture-defined data abort values for FSR ordered
 * in highest to lowest priority.
 */
#define DABT_TERMINAL				0x2
#define DABT_VECTOR				0x0	/* Obsolete */
#define DABT_ALIGN				0x1
#define DABT_EXT_XLATE_LEVEL1			0xC
#define DABT_EXT_XLATE_LEVEL2			0xE
#define DABT_XLATE_SECT				0x5
#define DABT_XLATE_PAGE				0x7
#define DABT_DOMAIN_SECT			0x9
#define DABT_DOMAIN_PAGE			0xB
#define DABT_PERM_SECT				0xD
#define DABT_PERM_PAGE				0xF
#define DABT_EXT_LFETCH_SECT			0x4
#define DABT_EXT_LFETCH_PAGE			0x6
#define DABT_EXT_NON_LFETCH_SECT		0x8
#define DABT_EXT_NON_LFETCH_PAGE		0xA

#define TASK_PGD(x)		(x)->space->pgd

#define STACK_ALIGNMENT				8

/* Kernel's data about the fault */
typedef struct fault_kdata {
	u32 faulty_pc;
	u32 fsr;
	u32 far;
	pte_t pte;
} __attribute__ ((__packed__)) fault_kdata_t;

void arch_hardware_flush(pgd_table_t *pgd);
void add_section_mapping_init(unsigned int paddr, unsigned int vaddr,
			      unsigned int size, unsigned int flags);

void add_boot_mapping(unsigned int paddr, unsigned int vaddr,
		      unsigned int size, unsigned int flags);

struct address_space;
int delete_page_tables(struct address_space *space);
int copy_user_tables(struct address_space *new, struct address_space *orig);
pgd_table_t *copy_page_tables(pgd_table_t *from);
void remap_as_pages(void *vstart, void *vend);

int pgd_count_pmds(pgd_table_t *pgd);
pgd_table_t *realloc_page_tables(void);
void remove_section_mapping(unsigned long vaddr);

void copy_pgds_by_vrange(pgd_table_t *to, pgd_table_t *from,
			 unsigned long start, unsigned long end);

#endif /* __V5_MM_H__ */
