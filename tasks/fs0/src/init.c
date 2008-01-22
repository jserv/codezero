/*
 * FS0 Initialisation.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <kdata.h>
#include <fs.h>
#include <string.h>
#include <stdio.h>
#include <l4/lib/list.h>
#include <init.h>
#include <blkdev/blkdev.h>

void initialise(void)
{
	struct block_device *bdev;

	request_initdata(&initdata);
	vfs_probe_fs(bdev);
}

