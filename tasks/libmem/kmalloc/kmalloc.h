#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include <mm/alloc_page.h>
#include <l4/lib/list.h>

/*
 * List member to keep track of free and unused regions in subpages.
 * Smallest unit it represents is one byte, but note that it is also
 * used for describing regions that span across multiple pages.
 */
struct km_area {
	struct list_head list;
	unsigned long vaddr;
	unsigned long size;
	int used;
	int pg_alloc_pages;	/* Means borrowed from alloc_page() */
};

extern struct list_head km_area_start;

/* Kmalloc initialisation */
void kmalloc_init(void);

/* Kmalloc allocation functions */
void *kmalloc(int size);
void *kzalloc(int size);
int kfree(void *vaddr);

#endif /* __KMALLOC_H__ */

