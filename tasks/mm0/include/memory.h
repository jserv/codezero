/*
 * Physical page descriptor
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <vm_area.h>
#include <init.h>
#include <physmem.h>

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
void *pager_map_file_range(struct vm_file *f, unsigned long byte_offset,
			   unsigned long size);
void *pager_validate_map_user_range2(struct tcb *user, void *userptr,
				    unsigned long size, unsigned int vm_flags);
#endif /* __MEMORY_H__ */
