/*
 * Requesting system information from kernel during init.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <stdio.h>
#include <l4lib/arch/syscalls.h>
#include <kdata.h>
#include <string.h>
#include INC_API(kip.h)
#include <kmalloc/kmalloc.h>

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

int request_initdata(struct initdata *initdata)
{
	int err;
	int bootdesc_size;

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
	printf("FATAL: %s failed during initialisation. exiting.\n", __TASKNAME__);
	return err;
}

