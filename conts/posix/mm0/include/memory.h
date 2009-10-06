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
#include <linker.h>

#define PAGER_MMAP_SEGMENT		SZ_4MB
#define PAGER_MMAP_START		(page_align_up(__stack))
#define PAGER_MMAP_END			(PAGER_MMAP_START + PAGER_MMAP_SEGMENT)

void init_mm_descriptors(struct page_bitmap *page_map,
			 struct bootdesc *bootdesc, struct membank *membank);
void init_physmem(void);

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

void *l4_new_virtual(int npages);
void *l4_del_virtual(void *virt, int npages);

#endif /* __MEMORY_H__ */
