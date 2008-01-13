/*
 * Prototypes for mmap/munmap functions that do the actual work.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __MM0_MMAP_H__
#define __MM0_MMAP_H__

#include <task.h>
#include <vm_area.h>

int do_munmap(void *vaddr, unsigned long size, struct tcb *task);

int do_mmap(struct vm_file *mapfile, unsigned long f_offset, struct tcb *t,
	    unsigned long map_address, unsigned int flags, unsigned int pages);

#endif /* __MM0_MMAP_H__ */
