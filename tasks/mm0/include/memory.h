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

int do_mmap(struct vm_file *mapfile, unsigned long f_offset, struct tcb *t,
	    unsigned long map_address, unsigned int flags, unsigned int pages);

#endif /* __MEMORY_H__ */
