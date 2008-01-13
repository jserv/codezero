/*
 * Boot time memory initialisation and memory allocator interface.
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */
#ifndef __GENERIC_PHYSMEM_H__
#define __GENERIC_PHYSMEM_H__

#include <l4/lib/list.h>
#include INC_PLAT(offsets.h)
#include INC_GLUE(memory.h)

#define PHYSMEM_TOTAL_PAGES	((PHYS_MEM_END - PHYS_MEM_START) >> PAGE_BITS)

/* A compact memory descriptor to determine used/unused pages in the system */
struct page_bitmap {
	unsigned long pfn_start;
	unsigned long pfn_end;
	unsigned int map[PHYSMEM_TOTAL_PAGES >> 5];
};

/* Describes a portion of physical memory. */
struct memdesc {
	unsigned int start;
	unsigned int end;
	unsigned int free_cur;
	unsigned int free_end;
	unsigned int numpages;
};

#if defined(__KERNEL__)
/* Describes bitmap of used/unused state for all physical pages */
extern struct page_bitmap page_map;
extern struct memdesc physmem;
#endif

/* Sets the global page map as used/unused. Aligns input when needed. */
int set_page_map(unsigned long start, int numpages, int val);

/* Memory allocator interface */
void physmem_init(void);
void memory_init(void);

#endif /* __GENERIC_PHYSMEM_H__ */
