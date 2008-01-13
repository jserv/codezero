/*
 * ARM v5-specific virtual memory details
 *
 * Copyright (C) 2005 Bahadir Balban
 */
#ifndef __V5__MM__H__
#define __V5__MM__H__

/* TODO: Change all LEVEL1_ prefix to PGD and
 * shorten the macros in general */
#define	LEVEL1_PAGETABLE_SIZE			SZ_4K * 4
#define LEVEL1_PAGETABLE_NUMENT			SZ_4K
#define	LEVEL1_PTE_TYPE_MASK			0x3
#define	LEVEL1_COARSE_ALIGN_MASK		0xFFFFFC00
#define	LEVEL1_SECTION_ALIGN_MASK		0xFFF00000
#define	LEVEL1_FINE_ALIGN_MASK			0xFFFFF000
#define	LEVEL1_TYPE_FAULT			0
#define	LEVEL1_TYPE_COARSE			1
#define	LEVEL1_TYPE_SECTION			2
#define	LEVEL1_TYPE_FINE			3

#define LEVEL2_TYPE_FAULT			0
#define LEVEL2_TYPE_LARGE			1
#define LEVEL2_TYPE_SMALL			2
#define LEVEL2_TYPE_TINY			3

/* Permission field offsets */
#define SECTION_AP0				10

#define PMD_SIZE				SZ_1K
#define PMD_NUM_PAGES				256

/* Applies for both small and large pages */
#define PAGE_AP0				4
#define PAGE_AP1				6
#define	PAGE_AP2				8
#define PAGE_AP3				10

/* Permission values with rom and sys bits ignored */
#define SVC_RW_USR_NONE				1
#define SVC_RW_USR_RO				2
#define SVC_RW_USR_RW				3

#define CACHEABILITY				3
#define BUFFERABILITY				4
#define cacheable				(1 << CACHEABILITY)
#define bufferable				(1 << BUFFERABILITY)

static inline void
__add_section_mapping_init(unsigned int paddr, unsigned int vaddr,
			   unsigned int size, unsigned int flags)
{
}
static inline void
add_section_mapping_init(unsigned int paddr, unsigned int vaddr,
			 unsigned int size, unsigned int flags)
{
}
static inline void
add_mapping(unsigned int paddr, unsigned int vaddr,
	    unsigned int size, unsigned int flags)
{
}

static inline void remove_mapping(unsigned int vaddr)
{
}
#endif /* __V5__MM__H__ */
