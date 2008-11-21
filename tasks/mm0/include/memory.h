/*
 * Physical page descriptor
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <vm_area.h>
#include <init.h>

struct membank {
	unsigned long start;
	unsigned long end;
	unsigned long free;
	struct page *page_array;
};
extern struct membank membank[];

void init_mm_descriptors(struct page_bitmap *page_map,
			 struct bootdesc *bootdesc, struct membank *membank);
void init_physmem(struct initdata *initdata, struct membank *membank);

int pager_address_pool_init(void);
void *pager_new_address(int npages);
int pager_delete_address(void *virt_addr, int npages);
void *pager_map_pages(struct vm_file *f, unsigned long page_offset, unsigned long npages);
void pager_unmap_pages(void *addr, unsigned long npages);
void *pager_map_page(struct vm_file *f, unsigned long page_offset);
void pager_unmap_page(void *addr);

#endif /* __MEMORY_H__ */
