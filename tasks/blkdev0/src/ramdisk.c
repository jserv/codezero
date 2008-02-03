/*
 * A basic ramdisk implementation.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
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
extern char _start_ramdisk0[];
extern char _end_ramdisk0[];
extern char _start_ramdisk1[];
extern char _end_ramdisk1[];

struct ramdisk_data {
	u64 base;
	u64 end;
};

struct ramdisk_data rddata[2];

__attribute__((section(".data.sfs"))) char sfsdisk[SZ_16MB];

int ramdisk_init(struct block_device *ramdisk)
{
	struct ramdisk_data *rddata = ramdisk->private;

	if (!strcmp("ramdisk0", ramdisk->name)) {
		rddata->base = (u64)(unsigned long)_start_ramdisk0;
		rddata->end = (u64)(unsigned long)_end_ramdisk0;
		ramdisk->size = (u64)((unsigned long)_end_ramdisk0 -
				      (unsigned long)_start_ramdisk0);
	} else if (!strcmp("ramdisk1", ramdisk->name)) {
		rddata->base = (u64)(unsigned long)_start_ramdisk1;
		rddata->end = (u64)(unsigned long)_end_ramdisk1;
		ramdisk->size = (u64)((unsigned long)_end_ramdisk1 -
				      (unsigned long)_start_ramdisk1);
	} else
		return -1;

	return 0;
}

int ramdisk_open(struct block_device *ramdisk)
{
	return 0;
}

int ramdisk_read(struct block_device *b, unsigned long offset, int size, void *buf)
{
	struct ramdisk_data *data = b->private;
	void *src = (void *)(unsigned long)(data->base + offset);

	memcpy(buf, src, size);
	return 0;
}

int ramdisk_write(struct block_device *b, unsigned long offset,
		  int size, void *buf)
{
	struct ramdisk_data *data = b->private;
	void *dst = (void *)(unsigned long)(data->base + offset);

	memcpy(dst, buf, size);
	return 0;
}

int ramdisk_readpage(struct block_device *b, unsigned long pfn, void *buf)
{
	ramdisk_read(b, __pfn_to_addr(pfn), PAGE_SIZE, buf);
	return 0;
}

int ramdisk_writepage(struct block_device *b, unsigned long pfn, void *buf)
{
	ramdisk_write(b, __pfn_to_addr(pfn), PAGE_SIZE, buf);
	return 0;
}

struct block_device ramdisk[2] = {
	[0] = {
		.name = "ramdisk0",
		.private = &rddata[0],
		.ops = {
			.init = ramdisk_init,
			.open = ramdisk_open,
			.read = ramdisk_read,
			.write = ramdisk_write,
			.read_page = ramdisk_readpage,
			.write_page = ramdisk_writepage,
		},
	},
	[1] = {
		.name = "ramdisk1",
		.private = &rddata[1],
		.ops = {
			.init = ramdisk_init,
			.open = ramdisk_open,
			.read = ramdisk_read,
			.write = ramdisk_write,
			.read_page = ramdisk_readpage,
			.write_page = ramdisk_writepage,
		},
	}
};

