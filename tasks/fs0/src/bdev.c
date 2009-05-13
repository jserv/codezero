/*
 * This is just to allocate some memory as a block device.
 */
#include <l4/macros.h>
#include <memfs/memfs.h>

extern char _start_bdev[];
extern char _end_bdev[];

__attribute__((section(".data.memfs"))) char blockdevice[MEMFS_TOTAL_SIZE];

void *vfs_rootdev_open(void)
{
	return (void *)_start_bdev;
}
