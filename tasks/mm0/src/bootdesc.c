/*
 * Reading of bootdesc forged at build time.
 *
 * Copyright (C) 2007 - 2009 Bahadir Balban
 */

#include <bootdesc.h>
#include <bootm.h>
#include <init.h>
#include <l4lib/arch/syslib.h>

extern unsigned long _end[];

void read_bootdesc(struct initdata *initdata)
{
	int npages;
	struct bootdesc *bootdesc;

	/*
	 * End of the executable image is where bootdesc resides
	 */
	bootdesc = (struct bootdesc *)_end;

	/* Check if bootdesc spans across pages, and how many */
	npages = __pfn((((unsigned long)bootdesc +
			 bootdesc->desc_size)
			& ~PAGE_MASK) -
		       ((unsigned long)bootdesc & ~PAGE_MASK));

	if (npages > 0)
		l4_map_helper(virt_to_phys((void *)page_align_up(_end)),
			      PAGE_SIZE * npages);

	/* Allocate bootdesc sized structure */
	initdata->bootdesc = alloc_bootmem(bootdesc->desc_size, 0);

	/* Copy bootdesc to initdata */
	memcpy(initdata->bootdesc, bootdesc,
	       bootdesc->desc_size);

	if (npages > 0)
		l4_unmap_helper((void *)page_align_up(_end),
				PAGE_SIZE * npages);
}
