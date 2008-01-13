#ifndef __BLOCK_DEV_H__
#define __BLOCK_DEV_H__

void bdev_open(void);
void bdev_readpage(unsigned long offset, void *buf);
void bdev_writepage(unsigned long offset, void *buf);

#endif /* __BLOCK_DEV_H__ */
