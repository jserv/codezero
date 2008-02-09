/*
 * Data that comes from the kernel.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __MM_KDATA_H__
#define __MM_KDATA_H__

#include <l4/macros.h>
#include <l4/config.h>
#include <l4/types.h>
#include <l4/generic/physmem.h>
#include INC_PLAT(offsets.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(memlayout.h)
#include INC_ARCH(bootdesc.h)
#include <vm_area.h>

struct initdata {
	struct bootdesc *bootdesc;
	struct list_head boot_file_list;
	struct page_bitmap page_map;
};

extern struct initdata initdata;

int request_initdata(struct initdata *i);

#endif /* __MM_KDATA_H__ */
