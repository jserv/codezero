/*
 * Virtual memory area descriptors. No page cache yet.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __VM_AREA_H__
#define __VM_AREA_H__

#include <stdio.h>
#include <task.h>
#include <l4/macros.h>
#include <l4/config.h>
#include <l4/types.h>
#include <arch/mm.h>
#include <lib/spinlock.h>

/* Protection flags */
#define VM_NONE				(1 << 0)
#define VM_READ				(1 << 1)
#define VM_WRITE			(1 << 2)
#define VM_EXEC				(1 << 3)
#define VM_PROT_MASK			(VM_READ | VM_WRITE | VM_EXEC)
#define VM_SWAPPED			(1 << 4)

/* VMA flags */
#define VMA_SHARED			(1 << 3)
/* VMA that's not file-backed, always ZI */
#define VMA_ANON			(1 << 4)
/* Private copy of a file VMA, can be ZI */
#define VMA_COW				(1 << 5)
/* This marks shadow vmas */
#define VMA_SHADOW			(1 << 6)

struct page {
	int count;		/* Refcount */
	struct spinlock lock;	/* Page lock. */
	struct list_head list;  /* For list of a file's in-memory pages */
	unsigned long virtual;	/* If refs >1, first mapper's virtual address */
	struct vm_file *owner;	/* The file it belongs to */
	unsigned int flags;	/* Flags associated with the page. */
	unsigned long f_offset; /* The offset page resides in its owner */
};
extern struct page *page_array;

#define page_refcnt(x)		((x)->count + 1)
#define virtual(x)		((x)->virtual)
#define phys_to_page(x)		(page_array + __pfn(x))
#define page_to_phys(x)		__pfn_to_addr((((void *)x) - \
					       (void *)page_array) \
					      / sizeof(struct page))

/* Fault data specific to this task + ptr to kernel's data */
struct fault_data {
	fault_kdata_t *kdata;		/* Generic data flonged by the kernel */
	unsigned int reason;		/* Generic fault reason flags */
	unsigned int address;		/* Aborted address */
	struct vm_area *vma;		/* Inittask-related fault data */
	struct tcb *task;		/* Inittask-related fault data */
};

struct vm_pager_ops {
	int (*read_page)(struct vm_file *f, unsigned long f_offset, void *pagebuf);
	int (*write_page)(struct vm_file *f, unsigned long f_offset, void *pagebuf);
};

/* Describes the pager task that handles a vm_area. */
struct vm_pager {
	struct vm_pager_ops ops;	/* The ops the pager does on area */
};

/*
 * Describes the in-memory representation of a file. This could
 * point at a file or another resource, e.g. a device area or swapper space.
 */
struct vm_file {
	int refcnt;
	unsigned long vnum;	/* Vnode number */
	unsigned long length;
	struct list_head list; /* List of all vm files in memory */

	/* This is the cache of physical pages that this file has in memory. */
	struct list_head page_cache_list;
	struct vm_pager *pager;
};


/*
 * Describes a virtually contiguous chunk of memory region in a task. It covers
 * a unique virtual address area within its task, meaning that it does not
 * overlap with other regions in the same task. The region could be backed by a
 * file or various other resources. This is managed by the region's pager.
 */
struct vm_area {
	struct list_head list;		/* Vma list */
	struct list_head shadow_list;	/* Head for shadow list. See fault.c */
	unsigned long pfn_start;	/* Region start virtual pfn */
	unsigned long pfn_end;		/* Region end virtual pfn, exclusive */
	unsigned long flags;		/* Protection flags. */
	unsigned long f_offset;		/* File offset in pfns */
	struct vm_file *owner;		/* File that backs the area. */
};

static inline struct vm_area *find_vma(unsigned long addr,
				       struct list_head *vm_area_list)
{
	struct vm_area *vma;
	unsigned long pfn = __pfn(addr);

	list_for_each_entry(vma, vm_area_list, list)
		if ((pfn >= vma->pfn_start) && (pfn < vma->pfn_end))
			return vma;
	return 0;
}

/* Adds a page to its vmfile's page cache in order of offset. */
int insert_page_olist(struct page *this, struct vm_file *f);

/* Pagers */
extern struct vm_pager default_file_pager;
extern struct vm_pager boot_file_pager;
extern struct vm_pager swap_pager;


/* Main page fault entry point */
void page_fault_handler(l4id_t tid, fault_kdata_t *fkdata);

#endif /* __VM_AREA_H__ */
