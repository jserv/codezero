/*
 * Simple linked-list based kernel memory allocator.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <l4/config.h>
#include <l4/macros.h>
#include <l4/types.h>
#include INC_GLUE(memory.h)
#include INC_GLUE(memlayout.h)
#include INC_SUBARCH(mm.h)
#include <l4lib/arch/syscalls.h>
#include <l4/lib/list.h>
#include <kmalloc/kmalloc.h>
#include <mm/alloc_page.h>

/* Initial free area descriptor.
 *
 * Basic description of how free areas are tracked:
 *
 * A km_area marked with pg_alloc_pages means it is located at the beginning
 * of a new page allocation, and it is the first struct to describe those
 * allocated page(s).
 *
 * If, for all subpage_areas, pg_alloc_pages = {SA, SB, ..., SZ}, and `fragments
 * of pg_alloc_pages' = {sa(n), sb(n), ..., sz(n)} where n is the sequence number
 * of that fragment, and for each SX, SX = sx(1), and "->" denotes "next"
 * pointer relationship, on a random occasion, the areas could look like this:
 *
 * SA->sa(2)->sa(3)->SB->sb(2)->SC->SD->SE->se(2)->se(3)->se(4)
 *
 * With regard to all alloc/free functions defined below, in this example's
 * context, sa(1..3) can merge if any adjacent pair of them are free. Whereas if
 * adjacent(SC,SD) were true, SC and SD cannot be merged even if they are both
 * free, because they are pg_alloc_pages. Also, for each SX, it can be freed IFF
 * it is the only element in SX, and it is free. For instance, each of SC or SD
 * can be individually freed, provided they are marked unused.
 *
 * We could have used a bucket for each, e.g:
 *
 * SA->sa(2)->sa(3)
 * |
 * v
 * SB->sb(2)->sb(3)
 * |
 * v
 * SC
 * |
 * v
 * SD
 *
 * etc. But the original is simple enough for now and does the job.
 *
 */

struct list_head km_area_start;

/*
 * Initialises a km_area descriptor according to the free area parameters
 * supplied along with it. @ppage = pointer to start of free memory.
 * @npages = number of pages the region contains. @km_areas = head of the list
 * of km_areas on the system that belongs to kmalloc.
 */
void kmalloc_add_new_pages(void *ppage, int npages, struct list_head *km_areas)
{
	struct km_area *new = (struct km_area *)ppage;

	new->vaddr = (unsigned long)ppage + sizeof(struct km_area);
	new->size = (npages * PAGE_SIZE) - sizeof(struct km_area);
	new->used = 0;
	new->pg_alloc_pages = npages;
	INIT_LIST_HEAD(&new->list);

	/*
	 * The first entry is a pg_alloc_pages. Adding the new pg_alloc_pages
	 * in tail ensures each pg_alloc_pages are adjacent, and their
	 * children are never intermixed.
	 */
	list_add_tail(&new->list, km_areas);
}

#define KM_INIT_PAGES			3
void kmalloc_init()
{
	/* Initially allocated pages with one big free km_area */
	void *ppage = l4_map_helper(alloc_page(KM_INIT_PAGES),
				    KM_INIT_PAGES);
	struct km_area *new = (struct km_area *)ppage;

	BUG_ON(!new);
	new->vaddr = (unsigned long)ppage + sizeof(struct km_area);
	new->size = (KM_INIT_PAGES * PAGE_SIZE)
		    - sizeof(struct km_area);
	new->used = 0;
	new->pg_alloc_pages = KM_INIT_PAGES;
	INIT_LIST_HEAD(&new->list);
	INIT_LIST_HEAD(&km_area_start);

	/* Add the first area to the global list head */
	list_add(&new->list, &km_area_start);

	/* NOTE: If needed, initialise mutex here */
}

/*
 * Given a free list, finds a free region of requested size plus one subpage
 * area descriptor. Allocates and initialises the new descriptor, adds it to
 * the list and returns it.
 */
static struct km_area *
find_free_km_area(int size, struct list_head *km_areas)
{
	struct km_area *new;
	struct km_area *area;
	const unsigned long alignment_extra_max = SZ_WORD - 1;
	int alignment_used = 0, alignment_unused = 0;

	/* The minimum size needed if the area will be divided into two */
	int dividable_size = size + sizeof(struct km_area)
			     + alignment_extra_max;

	list_for_each_entry (area, km_areas, list) {
	  	/* Is this a free region that fits? */
		if ((area->size) >= dividable_size && !area->used) {
			unsigned long addr, addr_aligned;

			/*
			 * Cut the free area from the end, as much as
			 * we want to use
			 */
			area->size -= size + sizeof(struct km_area);

			addr = (area->vaddr + area->size);
			addr_aligned = align_up(addr, SZ_WORD);
			alignment_used = addr_aligned - addr;
			alignment_unused = alignment_extra_max
					   - alignment_used;

			/*
			 * Add the extra bit that's skipped for alignment
			 * to original subpage
			 */
			area->size += alignment_used;

			/*
			 * Allocate the new link structure at the end
			 * of the free area shortened previously.
			 */
			new = (struct km_area *)addr_aligned;

			/*
			 * Actual allocated memory starts after subpage
			 * descriptor
			 */
			new->vaddr = (unsigned long)new
				     + sizeof(struct km_area);
			new->size = size + sizeof(struct km_area)
				    + alignment_unused;
			new->used = 1;

			/* Divides other allocated page(s) */
			new->pg_alloc_pages = 0;

			/* Add used region to the page area list */
			INIT_LIST_HEAD(&new->list);
			list_add(&new->list, &area->list);
			return new;

		} else if (area->size < dividable_size &&
			   area->size >= size && !area->used) {
			/*
			 * Area not at dividable size but can satisfy request,
			 * so it's simply returned.
			 */
			area->used = 1;
			return area;
		}
	}
	/* Traversed all areas and can't satisfy request. */
	return 0;
}

/*
 * Allocate, initialise a km_area along with its free memory of minimum
 * size as @size, and add it to km_area list.
 */
static int
kmalloc_get_free_pages(int size, struct list_head *km_areas)
{
	int totalsize = size + sizeof(struct km_area) * 2;
	int npages = totalsize / PAGE_SIZE;
	void *ppage;

	if (totalsize & PAGE_MASK)
		npages++;

	if ((ppage = l4_map_helper(alloc_page(npages), npages)) == 0)
		/* TODO: Return specific error code, e.g. ENOMEM */
		return -1;

	BUG_ON((npages * PAGE_SIZE) < (size + sizeof(struct km_area)));

	kmalloc_add_new_pages(ppage, npages, km_areas);
	return 0;
}

/*
 * Linked list based kernel memory allocator. This has the simplicity of
 * allocating list structures together with the requested memory area. This
 * can't be done with the page allocator, because it works in page-size chunks.
 * In kmalloc we can allocate more fine-grain sizes, so a link structure can
 * also be embedded together with requested data.
 */

/* Allocates given @size, requests more free pages if free areas depleted. */
void *kmalloc(int size)
{
	struct km_area *new_area;
	void *allocation;

	/* NOTE: If needed, lock mutex here */
	new_area = find_free_km_area(size, &km_area_start);
	if (!new_area) {
		if (kmalloc_get_free_pages(size, &km_area_start) < 0) {
			allocation = 0;
			goto out;
		}
		else
			new_area = find_free_km_area(size, &km_area_start);
	}
	BUG_ON(!new_area);
 	allocation = (void *)new_area->vaddr;
out:
	/* NOTE: If locked, unlock mutex here */
	return allocation;
}

/* kmalloc with zero initialised memory */
void *kzalloc(int size)
{
	void *mem = kmalloc(size);
	if (mem)
		memset(mem, 0, size);
	return mem;
}

void km_free_empty_pages(struct km_area *free_area)
{
	unsigned long wholesize;

	/* Not allocated from page allocator */
	if (!free_area->pg_alloc_pages)
		return;

	/* The first km_area in memory from the page allocator: */

	/* Must be on a page boundary */
	BUG_ON((unsigned long)free_area & PAGE_MASK);

	/* Must be unused */
	BUG_ON(free_area->used);

	/* Must be whole, (i.e. not divided into other km_areas) */
	wholesize = free_area->pg_alloc_pages * PAGE_SIZE;
	if ((free_area->size + sizeof(struct km_area)) < wholesize)
		return;

	/* Must have at least PAGE_SIZE size, when itself included */
	BUG_ON(free_area->size < (PAGE_SIZE - sizeof(struct km_area)));

	/* Its size must be a multiple of PAGE_SIZE, when itself included */
	if ((free_area->size + sizeof(struct km_area)) & PAGE_MASK) {
		printk("Error: free_area->size: 0x%lu, with km_area_struct:"
		       " 0x%lu, PAGE_MASK: 0x%x\n", free_area->size,
		       free_area->size + sizeof(struct km_area), PAGE_MASK);
		BUG();
	}
	list_del(&free_area->list);

	/* And finally must be freed without problems */
	BUG_ON(free_page(l4_unmap_helper(free_area, __pfn(wholesize))) < 0);
	return;
}

struct km_area *km_merge_free_areas(struct km_area *before,
				    struct km_area *after)
{

	/*
	 * If `after' has pg_alloc_pages set, it means it can't be merged and
	 * has to be returned explicitly to the page allocator.
	 */
	if (after->pg_alloc_pages)
		return 0;

	BUG_ON(before->vaddr + before->size != after->vaddr);
	BUG_ON(before->used || after->used)
	BUG_ON(before == after);

	/*
	 * km_area structures are at the beginning of the memory
	 * areas they describe. By simply merging them with another
	 * area they're effectively freed.
	 */
	before->size += after->size + sizeof(struct km_area);
	list_del(&after->list);
	return before;
}


int find_and_free_km_area(void *vaddr, struct list_head *areas)
{
	struct km_area *area, *prev, *next, *merged;

	if (!vaddr)		/* A well-known invalid address */
		return -1;

	list_for_each_entry(area, areas, list)
		if (area->vaddr == (unsigned long)vaddr && area->used)
			goto found;

	/* Area not found */
	return -1;

found:

	area->used = 0;

	/* Now merge with adjacent areas if possible */
	if (area->list.prev != areas) {
		prev = list_entry(area->list.prev, struct km_area, list);
		if (!prev->used)
			if ((merged = km_merge_free_areas(prev, area)))
				area = merged;
	}
	if (area->list.next != areas) {
		next = list_entry(area->list.next, struct km_area, list);
		if (!next->used)
			if ((merged = km_merge_free_areas(prev, area)))
				area = merged;
	}

	/*
	 * After freeing and all possible merging, try returning region back
	 * to page allocator.
	 */
	km_free_empty_pages(area);
	return 0;
}

int kfree(void *virtual)
{
	int ret;

	/* NOTE: If needed, lock mutex here */
	ret = find_and_free_km_area(virtual, &km_area_start);
	/* NOTE: If locked, unlock mutex here */

	return ret;
}

