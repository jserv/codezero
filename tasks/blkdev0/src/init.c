

#include <blkdev.h>
#include <ramdisk.h>


void init_blkdev(void)
{
	ramdisk.ops.open(&ramdisk);
}
