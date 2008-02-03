

#include <blkdev.h>
#include <ramdisk.h>


void init_blkdev(void)
{
	ramdisk[0].ops.init(&ramdisk[0]);
	ramdisk[1].ops.init(&ramdisk[1]);
}
