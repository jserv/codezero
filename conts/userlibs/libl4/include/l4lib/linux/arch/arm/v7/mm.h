/*
 * v7 memory management definitions
 *
 * Copyright (C) 2010 B Labs Ltd.
 * Written by Bahadir Balban
 */

#ifndef __V7_MM_H__
#define __V7_MM_H__

/* Generic definitions used across the kernel */
#define VIRT_MEM_START			0
#define VIRT_MEM_END			0xFFFFFFFF

/* Non-global first level descriptor definitions */
#define TASK_PGD_SIZE_MAP4GB			SZ_16K
#define TASK_PGD_SIZE_MAP2GB			SZ_8K
#define TASK_PGD_SIZE_MAP1GB			SZ_4K
#define TASK_PGD_SIZE_MAP512MB			(SZ_1K * 2)
#define TASK_PGD_SIZE_MAP256MB			SZ_1K
#define TASK_PGD_SIZE_MAP128MB			512
#define TASK_PGD_SIZE_MAP64MB			256
#define TASK_PGD_SIZE_MAP32MB			128

/* Any virtual mapping above this value goes to the global table */
#define PGD_GLOBAL_BOUNDARY		0x80000000

/* Task-specific page table, userspace private + shared memory mappings */
#define PGD_ENTRY_TOTAL			(TASK_PGD_SIZE_MAP2GB >> 2)
#define PGD_SIZE			(TASK_PGD_SIZE_MAP2GB)

/* Global page table size UTCB + kernel + device mappings */
#define PGD_GLOBAL_SIZE			SZ_16K
#define PGD_GLOBAL_ENTRY_TOTAL		(PGD_GLOBAL_SIZE >> 2)

#if !defined(__LINUX_CONTAINER__)
#define PMD_SIZE			SZ_1K
#endif
#define PMD_ENTRY_TOTAL			256
#define PMD_MAP_SIZE			SZ_1MB

/* FIXME: Check these shifts/masks are correct */
#define PGD_INDEX_MASK			0x3FFC
#define PGD_INDEX_SHIFT			18

#define PMD_INDEX_MASK			0x3FC
#define PMD_INDEX_SHIFT			10

/*
 * These are indices into arrays with pmd_t or pte_t sized elements,
 * therefore the index must be divided by appropriate element size
 */
#define PGD_INDEX(x)		(((((unsigned long)(x)) >> PGD_INDEX_SHIFT) \
				  & PGD_INDEX_MASK) / sizeof(pmd_t))

/* Strip out the page offset in this megabyte from a total of 256 pages. */
#define PMD_INDEX(x)		(((((unsigned long)(x)) >> PMD_INDEX_SHIFT) \
				  & PMD_INDEX_MASK) / sizeof (pte_t))

#if !defined (__ASSEMBLY__) && !defined (__LINUX_CONTAINER__)
/* Type-checkable page table elements */
typedef u32 pmd_t;
typedef u32 pte_t;

/* Page global directory made up of pmd_t entries */
typedef struct page_table_directory {
	pmd_t entry[PGD_GLOBAL_ENTRY_TOTAL];
} pgd_global_table_t;

/* Page non-global directory */
typedef struct task_page_table_directory {
	pmd_t entry[PGD_ENTRY_TOTAL];
} pgd_table_t;

/* Page middle directory made up of pte_t entries */
typedef struct pmd_table {
	pte_t entry[PMD_ENTRY_TOTAL];
} pmd_table_t;

extern pgd_table_t init_pgd;
extern pgd_global_table_t init_global_pgd;

#endif /* !defined(__ASSEMBLY__) */

/* PMD definitions (2nd level page tables) */
#define PMD_ALIGN_MASK			(~(PMD_SIZE - 1))
#define PMD_TYPE_FAULT			0x0
#define PMD_TYPE_PMD			0x1
#define PMD_TYPE_SECTION		0x2
#define PMD_TYPE_MASK			0x3
#define PMD_DOMAIN_SHIFT		5		/* Domain field on PGD entry */
#define PMD_DOMAIN_MASK			0x000001E0	/* Domain mask on PGD entry */
#define PMD_NS_BIT			3		/*  Non-secure memory */

/* First level Section definitions */
#define SECT_MAP_SIZE			SZ_1MB	/* Section base address alignment */
#define SECT_NS_BIT			19
#define SECT_SUPER_BIT			18
#define SECT_NG_BIT			17
#define SECT_SHAREABLE_BIT		16
#define SECT_AP2_BIT			15
#define SECT_TEX2_BIT			14
#define SECT_TEX1_BIT			13
#define SECT_TEX0_BIT			12
#define SECT_AP1_BIT			11
#define SECT_AP0_BIT			10
#define SECT_DOMAIN_SHIFT		5
#define SECT_XN_BIT			4
#define SECT_CACHE_BIT			3
#define SECT_BUFFER_BIT			2

#if !defined (__LINUX_CONTAINER__)
/* Second level entry (PTE) definitions */
#define PTE_TYPE_MASK			0x2
#define PTE_TYPE_FAULT			0
#define PTE_TYPE_LARGE			1
#define PTE_TYPE_SMALL			2
#endif

#define PTE_XN_BIT			0
#define PTE_BUFFER_BIT			2
#define PTE_CACHE_BIT			3
#define PTE_AP0_BIT			4
#define PTE_AP1_BIT			5
#define PTE_TEX0_BIT			6
#define PTE_TEX1_BIT			7
#define PTE_TEX2_BIT			8
#define PTE_AP2_BIT			9
#define PTE_AP01_SHIFT			PTE_AP0_BIT
#define PTE_AP01_MASK			0x30

#define PTE_SHARE_BIT			10
#define PTE_NG_BIT			11

/* Domain access types */
#define DOMAIN_ACCESS_NONE		0
#define DOMAIN_ACCESS_CLIENT		1
#define DOMAIN_ACCESS_MANAGER		3

/* Simplified permission model definitions */
#define PTE_ACCESS_FLAG			PTE_AP0_BIT

/* Bits [1:0] map as AP[2], AP[1] */
#define AP_SIMPLE_USER_NONE_KERN_RW		0
#define AP_SIMPLE_USER_RW_KERN_RW		1
#define AP_SIMPLE_USER_NONE_KERN_RO		2
#define AP_SIMPLE_USER_RO_KERN_RO		3

/*
 * Generic page table flag meanings for v7:
 *
 * Note these are not hardware-defined bits,
 * they are defined by the kernel for
 * convenience.
 *
 * [WXCDU]
 * W = write, X = Exec, C = Cached, D = Device
 *
 * If !D it means Normal memory.
 * If !U it means kernel-only.
 * If !W it means read-only.
 *
 *  These are actually meaningful but unused
 *  individually, rather the combination of them
 *  are directly converted into HW pte.
 */
#define PTE_MAP_USER	(1 << 0)
#define PTE_MAP_DEVICE	(1 << 1)
#define PTE_MAP_CACHED	(1 << 2)
#define PTE_MAP_EXEC	(1 << 3)
#define PTE_MAP_WRITE	(1 << 4)

/* 0 would mean normal, uncached, kernel mapping */
#define PTE_MAP_FAULT	(1 << 5)

/*
 * v7-specific conversion of map flags
 */

/* In ARMv7 normal, wbwa, shareable, user-rw/kern-rw, xn=1 */
#define __MAP_USR_RW (PTE_MAP_USER | PTE_MAP_WRITE | PTE_MAP_CACHED)

/* Writeback cached. In ARMv7 normal, wbwa, shareable, user-ro/kern-ro, xn=1 */
#define __MAP_USR_RO	(PTE_MAP_USER | PTE_MAP_CACHED)

/* Writeback cached. In ARMv7 normal, wbwa, shareable, user-none/kern-rw, xn=1 */
#define __MAP_KERN_RW	(PTE_MAP_CACHED | PTE_MAP_WRITE)

/* Uncached. In ARMv7 device, uncached, shareable, user-rw/kern-rw, xn=1 */
#define __MAP_USR_IO	(PTE_MAP_USER | PTE_MAP_DEVICE | PTE_MAP_WRITE)

/* Uncached. In ARMv7 device, uncached, shareable, user-none/kern-rw, xn=1 */
#define __MAP_KERN_IO	(PTE_MAP_DEVICE | PTE_MAP_WRITE)

/* Writeback cached. In ARMv7 normal, wbwa, shareable, user-rw/kern-rw, xn=0 */
#define __MAP_USR_RWX	(PTE_MAP_USER | PTE_MAP_CACHED \
			 | PTE_MAP_WRITE | PTE_MAP_EXEC)

/* Writeback cached. In ARMv7 normal, wbwa, shareable, user-none/kern-rw, xn=0 */
#define __MAP_KERN_RWX	(PTE_MAP_CACHED | PTE_MAP_WRITE | PTE_MAP_EXEC)

/* Writeback cached. In ARMv7 normal, wbwa, shareable, user-ro/kern-ro, xn=0 */
#define __MAP_USR_RX	(PTE_MAP_USER | PTE_MAP_CACHED | PTE_MAP_EXEC)

/* Writeback cached. In ARMv7 normal, wbwa, shareable, user-none/kern-ro, xn=0 */
#define __MAP_KERN_RX	(PTE_MAP_CACHED | PTE_MAP_EXEC)

/* Fault/unmapped entry */
#define __MAP_FAULT	PTE_MAP_FAULT

/*
 * Shareability bit remapping on tex remap
 *
 * As an example to below, when a normal region has its
 * shareability bit set to 1, PRRR_NORMAL_S1_BIT remaps
 * and determines the final shareability status. E.g. if
 * PRRR_NORMAL_S1_BIT is set to 0, the region becomes
 * not shareable, even though the pte S bit == 1.
 * On Tex Remap, PRRR is the final decision point.
 */
#define PRRR_DEVICE_S0_BIT		16 /* Meaning of all device memory when S == 0 */
#define PRRR_DEVICE_S1_BIT		17 /* Meaning of all device memory when S == 1 */
#define PRRR_NORMAL_S0_BIT		18 /* Meaning of all normal memory when S == 0 */
#define PRRR_NORMAL_S1_BIT		19 /* Meaning of all normal memory when S == 1 */
#define PRRR_NOS_START_BIT		24
#define NMRR_OUTER_START_BIT		16
#define CACHEABLE_NONE			0
#define CACHEABLE_WBWA			1
#define CACHEABLE_WT_NOWA		2
#define CACHEABLE_WB_NOWA		3

/* Memory type values for tex remap registers */
#define MEMTYPE_ST_ORDERED		0
#define MEMTYPE_DEVICE			1
#define MEMTYPE_NORMAL			2

/* User-defined tex remap slots */
#define TEX_SLOT_NORMAL_UNCACHED	0
#define TEX_SLOT_NORMAL			1
#define TEX_SLOT_DEVICE_UNCACHED	2
#define TEX_SLOT_ST_ORDERED_UNCACHED	3

#define ASID_MASK			0xFF
#define ASID_GROUP_SHIFT		8
#define PROCID_SHIFT			8
#define PROCID_MASK			0xFFFFFF

#define TASK_ASID(x)	((x)->space->spid & ASID_MASK)
#define SPACE_ASID(x)	((x)->spid & ASID_MASK)
#define TASK_PROCID(x)	((x)->tid & PROCID_MASK)

#define PGD_GLOBAL_GET()	(kernel_resources.pgd_global)

/*
 *
 * Page table memory settings for translation table walk hardware:
 *
 * We assume write-back write-allocate, inner and outer
 * cacheable, inner shareable, not outer-shareable,
 * normal memory.
 *
 * ARMv7 VMSA (B3-114) says that the obscure IRGN[1:0]
 * mapping ensures same bit values for SMP and v7 base architecture,
 * however this is only partially true as seen by the WBWA bit
 * mapping differences.
 *
 * RGN values:
 * 00	Uncached
 * 01	WBWA
 * 10	WT
 * 11	WB_NOWA
 *
 * On below definitions both inner and outer cacheability bits
 * are assigned with the same cacheability values.
 */
							/*		     I
							 * 	 I	     R
							 * 	 R   R R     G
							 * 	 G N G G I   N
							 * 	 N O N N M   1
							 * 	 0 S 1|0 P S C */
#define PGD_MEMORY_NORMAL_WBWA_S_NOS			0x2B /* 00 1 0|1 0 1 1 */
#define PGD_MEMORY_NORMAL_WBWA_S_NOS_SMP		0x6A /* 01 1 0|1 0 1 0 */
#define PGD_MEMORY_NORMAL_WB_NOWA_S_NOS			0x3B /* 00 1 1|1 0 1 1 */
#define PGD_MEMORY_NORMAL_WB_NOWA_S_NOS_SMP		0x7B /* 01 1 1|1 0 1 1 */
#define PGD_MEMORY_NORMAL_WB_NOWA_S_OS			0x1B /* 00 0 1|1 0 1 1 */
#define PGD_MEMORY_NORMAL_WB_NOWA_S_OS_SMP		0x5B /* 01 0 1|1 0 1 1 */
#define PGD_MEMORY_NORMAL_UNCACHED_S_NOS		0x22 /* 00 1 0|0 0 1 0 */
#define PGD_MEMORY_NORMAL_UNCACHED_S_NOS_SMP		0x22 /* 00 1 0|0 0 1 0 */
#define PGD_MEMORY_NORMAL_WBWA_S_OS			0x0B /* 00 0 0|1 0 1 1 */
#define PGD_MEMORY_NORMAL_WBWA_S_OS_SMP			0x4A /* 01 0 0|1 0 1 0 */

/* Returns page table memory settings for ttb walk fetches */
unsigned int ttb_walk_mem_settings(void);


#if !defined (__ASSEMBLY__)

void v7_flags_prepare_pte(pte_t *pte, unsigned long phys,
			  unsigned long virt, unsigned int v7_pte_flags);
void section_set_access_simple(pmd_t *pmd, unsigned int perms);
void section_set_tex_remap_slot(pmd_t *pmd, int slot);
void v7_write_section(unsigned long paddr, unsigned long vaddr,
		      unsigned int section_flags, unsigned int asid);
int pte_get_access_simple(pte_t pte);
void tex_remap_setup_all_slots(void);

struct ktcb;
void arch_update_utcb(unsigned long utcb_address);
void arch_space_switch(struct ktcb *to);
void system_identify(void);

#endif /* !defined(__ASSEMBLY__) */

#endif /* __V7_MM_H__ */
