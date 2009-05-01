/*
 * Requesting system information from kernel during init.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <string.h>
#include <init.h>
#include INC_API(kip.h)
#include <lib/malloc.h>
#include <l4lib/arch/syscalls.h>

/* Kernel data acquired during initialisation */
struct initdata initdata;
#define BOOTDESC_PREALLOC_SIZE		128
static char bootdesc_memory[BOOTDESC_PREALLOC_SIZE];	/* 128 bytes */

void print_bootdesc(struct bootdesc *bd)
{
	for (int i = 0; i < bd->total_images; i++) {
		printf("Task Image: %d\n", i);
		printf("Name: %s\n", bd->images[i].name);
		printf("Start: 0x%x\n", bd->images[i].phys_start);
		printf("End: 0x%x\n", bd->images[i].phys_end);
	}
}

void print_pfn_range(int pfn, int size)
{
	unsigned int addr = pfn << PAGE_BITS;
	unsigned int end = (pfn + size) << PAGE_BITS;
	printf("Used: 0x%x - 0x%x\n", addr, end);
}

void print_page_map(struct page_bitmap *map)
{
	unsigned int start_pfn = 0;
	unsigned int total_used = 0;
	int numpages = 0;

	printf("Pages start at address 0x%x\n", map->pfn_start << PAGE_BITS);
	printf("Pages end at address 0x%x\n", map->pfn_end << PAGE_BITS);
	printf("The used page areas are:\n");
	for (int i = 0; i < (PHYSMEM_TOTAL_PAGES >> 5); i++) {
		for (int x = 0; x < WORD_BITS; x++) {
			if (map->map[i] & (1 << x)) { /* A used page found? */
				if (!start_pfn) /* First such page found? */
					start_pfn = (WORD_BITS * i) + x;
				total_used++;
				numpages++; /* Increase number of pages */
			} else { /* Either used pages ended or were never found */
				if (start_pfn) { /* We had a used page */
					/* Finished end of used range.
					 * Print and reset. */
					print_pfn_range(start_pfn, numpages);
					start_pfn = 0;
					numpages = 0;
				}
			}
		}
	}
	printf("Total of %d pages. %d Kbytes.\n", total_used, total_used << 2);
}


int request_initdata(struct initdata *initdata)
{
	int err;
	int bootdesc_size;

	/* Read all used physical page information in a bitmap. */
	if ((err = l4_kread(KDATA_PAGE_MAP, &initdata->page_map)) < 0) {
		printf("L4_kdata_read() call failed. Could not complete"
		       "KDATA_PAGE_MAP request.\n");
		goto error;
	}
	print_page_map(&initdata->page_map);

	/* Read the boot descriptor size */
	if ((err = l4_kread(KDATA_BOOTDESC_SIZE, &bootdesc_size)) < 0) {
		printf("L4_kdata_read() call failed. Could not complete"
		       "KDATA_BOOTDESC_SIZE request.\n");
		goto error;
	}

	if (bootdesc_size > BOOTDESC_PREALLOC_SIZE) {
		printf("Insufficient preallocated memory for bootdesc. "
		       "Size too big.\n");
		goto error;
	}
	/* Get preallocated bootdesc memory */
	initdata->bootdesc = (struct bootdesc *)&bootdesc_memory;

	/* Read the boot descriptor */
	if ((err = l4_kread(KDATA_BOOTDESC, initdata->bootdesc)) < 0) {
		printf("L4_kdata_read() call failed. Could not complete"
		       "KDATA_BOOTDESC request.\n");
		goto error;
	}

	return 0;

error:
	printf("FATAL: Inittask failed during initialisation. exiting.\n");
	return err;
}

