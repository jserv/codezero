/*
 * A basic ramdisk implementation.
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 * The ramdisk binary is embedded in the data section of the ramdisk device
 * executable. Read/writes simply occur to this area. The disk area could
 * have any filesystem layout e.g. romfs, which is irrelevant for this code.
 */
#include <blkdev.h>
#include <ramdisk.h>
#include <l4/macros.h>
#include <l4/config.h>
#include <l4/types.h>
#include <string.h>
#include INC_SUBARCH(mm.h)
#include INC_GLUE(memory.h)

/* Ramdisk section markers for ramdisk inside this executable image */
extern char _start_ramdisk[];
extern char _end_ramdisk[];
static unsigned long ramdisk_base;

void ramdisk_open(struct block_device *ramdisk)
{
	ramdisk_base = (unsigned long)_start_ramdisk;
	ramdisk->size = (unsigned long)_end_ramdisk -
			(unsigned long)_start_ramdisk;
}

void ramdisk_read(unsigned long offset, int size, void *buf)
{
	void *src = (void *)(ramdisk_base + offset);

	memcpy(buf, src, size);
}

void ramdisk_write(unsigned long offset, int size, void *buf)
{
	void *dst = (void *)(ramdisk_base + offset);

	memcpy(dst, buf, size);
}

void ramdisk_readpage(unsigned long pfn, void *buf)
{
	ramdisk_read(__pfn_to_addr(pfn), PAGE_SIZE, buf);
}

void ramdisk_writepage(unsigned long pfn, void *buf)
{
	ramdisk_write(__pfn_to_addr(pfn), PAGE_SIZE, buf);
}

struct block_device ramdisk = {
	.name = "ramdisk",
	.ops = {
		.open = ramdisk_open,
		.read = ramdisk_read,
		.write = ramdisk_write,
		.read_page = ramdisk_readpage,
		.write_page = ramdisk_writepage,
	},
};

