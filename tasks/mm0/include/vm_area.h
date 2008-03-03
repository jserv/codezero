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

/* VMA object type flags */
#define VMOBJ_SHADOW			(1 << 6)

struct page {
	int count;		/* Refcount */
	struct spinlock lock;	/* Page lock. */
	struct list_head list;  /* For list of a vm_object's in-memory pages */
	struct vm_object *owner;/* The vm_object the page belongs to */
	unsigned long virtual;	/* If refs >1, first mapper's virtual address */
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
	int (*page_in)(struct vm_object *vm_obj, unsigned long f_offset);
	int (*page_out)(struct vm_object *vm_obj, unsigned long f_offset);
};

/* Describes the pager task that handles a vm_area. */
struct vm_pager {
	struct vm_pager_ops ops;	/* The ops the pager does on area */
};

/*
 * Describes the in-memory representation of a resource. This could
 * point at a file or another resource, e.g. a device area, swapper space,
 * the anonymous internal state of a process, etc. This covers more than
 * just files, e.g. during a fork, captures the state of internal shared
 * copy of private pages for a process, which is really not a file.
 */
struct vm_object {
	int npages;		    /* Number of pages in memory */
	int vma_refcnt;		    /* Number of vmas that refer */
	int shadow_refcnt;	    /* Number of shadows that refer */
	struct list_head shadows;   /* List of vm objects that shadow this one */
	struct vm_object *orig_vma; /* Original object that this one shadows */
	unsigned int type;	    /* Defines the type of the object */
	struct list_head list;	    /* List of all vm objects in memory */
	struct list_head page_cache;/* List of in-memory pages */
	struct vm_pager *pager;		  /* The pager for this object */
	union private_data {		  /* Private data about the object */
		struct vm_file *file;	  /* VFS file-specific information */
	} priv;
};

/* In memory representation of a vfs file. */
struct vm_file {
	unsigned long vnum;
	unsigned long length;
};

/* To create per-vma vm_object lists */
struct vma_obj_list {
	struct list_head list;
	struct vm_object *obj;
}

/*
 * Describes a virtually contiguous chunk of memory region in a task. It covers
 * a unique virtual address area within its task, meaning that it does not
 * overlap with other regions in the same task. The region could be backed by a
 * file or various other resources. This is managed by the region's pager.
 *
 * COW: Upon copy-on-write, each copy-on-write instance creates a shadow of the
 * original vma which supersedes the original vma with its copied modified pages.
 * This creates a stack of shadow vmas, where the top vma's copy of pages
 * supersede the ones lower in the stack.
 */
struct vm_area {
	struct list_head list;		/* Per-task vma list */
	struct list_head vm_obj_list;	/* Head for vm_object list. */
	unsigned long pfn_start;	/* Region start virtual pfn */
	unsigned long pfn_end;		/* Region end virtual pfn, exclusive */
	unsigned long flags;		/* Protection flags. */
	unsigned long f_offset;		/* File offset in pfns */
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

/* Adds a page to its vm_objects's page cache in order of offset. */
int insert_page_olist(struct page *this, struct vm_object *vm_obj);

/* Pagers */
extern struct vm_pager default_file_pager;
extern struct vm_pager boot_file_pager;
extern struct vm_pager swap_pager;


/* Main page fault entry point */
void page_fault_handler(l4id_t tid, fault_kdata_t *fkdata);

#endif /* __VM_AREA_H__ */
