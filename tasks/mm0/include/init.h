/*
 * Data that comes from the kernel, and other init data.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __MM_INIT_H__
#define __MM_INIT_H__

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
	struct page_bitmap page_map;
	struct list_head boot_file_list;
};

extern struct initdata initdata;

int request_initdata(struct initdata *i);

void initialise(void);
int init_devzero(void);
struct vm_file *get_devzero(void);
int init_boot_files(struct initdata *initdata);

#endif /* __MM_INIT_H__ */
