#ifndef __BLKDEV_H__
#define __BLKDEV_H__

#include <l4lib/types.h>

struct block_device;

struct block_device_ops {
	int (*init)(struct block_device *bdev);
	int (*open)(struct block_device *bdev);
	int (*read)(struct block_device *bdev, unsigned long offset,
		    int size, void *buf);
	int (*write)(struct block_device *bdev, unsigned long offset,
		     int size, void *buf);
	int (*read_page)(struct block_device *bdev,
			 unsigned long pfn, void *buf);
	int (*write_page)(struct block_device *bdev,
			  unsigned long pfn, void *buf);
};

struct block_device {
	char *name;
	void *private;	/* Low-level device specific data */
	u64 size;
	struct block_device_ops ops;
};


void init_blkdev(void);

#endif /* __BLKDEV_H__ */
