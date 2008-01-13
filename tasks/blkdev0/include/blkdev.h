#ifndef __BLKDEV_H__
#define __BLKDEV_H__

struct block_device;

struct block_device_ops {
	void (*open)(struct block_device *bdev);
	void (*read)(unsigned long offset, int size, void *buf);
	void (*write)(unsigned long offset, int size, void *buf);
	void (*read_page)(unsigned long pfn, void *buf);
	void (*write_page)(unsigned long pfn, void *buf);
};

struct block_device {
	char *name;
	unsigned long size;
	struct block_device_ops ops;
};


void init_blkdev(void);

#endif /* __BLKDEV_H__ */
