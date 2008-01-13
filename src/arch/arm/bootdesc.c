/*
 * Reading of bootdesc forged at build time.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#include <l4/lib/printk.h>
#include <l4/lib/string.h>
#include <l4/generic/kmalloc.h>
#include <l4/generic/space.h>
#include INC_ARCH(linker.h)
#include INC_ARCH(bootdesc.h)
#include INC_GLUE(memory.h)
#include INC_PLAT(printascii.h)
#include INC_SUBARCH(mm.h)

struct bootdesc *bootdesc;

void copy_bootdesc()
{
	struct bootdesc *new = kzalloc(bootdesc->desc_size);

	memcpy(new, bootdesc, bootdesc->desc_size);
	remove_mapping((unsigned long)bootdesc);
	bootdesc = new;
}

void read_bootdesc(void)
{
	/*
	 * End of the kernel image is where bootdesc resides. Note this is
	 * not added to the page_map because it's meant to be discarded.
	 */
	add_mapping(virt_to_phys(_end), (unsigned long)_end, PAGE_SIZE,
		    MAP_USR_DEFAULT_FLAGS);

	/* Get original bootdesc */
	bootdesc = (struct bootdesc *)_end;

	/* Determine end of physical memory used by loaded images. */
	for (int i = 0; i < bootdesc->total_images; i++)
		if (bootdesc->images[i].phys_end > __svc_images_end)
			__svc_images_end = bootdesc->images[i].phys_end;
}

