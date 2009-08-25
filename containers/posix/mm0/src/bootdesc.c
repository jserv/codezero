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
extern unsigned long pager_offset;

void read_bootdesc(struct initdata *initdata)
{
	int npages;
	struct bootdesc *bootdesc;

	/*
	 * End of the executable image is where bootdesc resides
	 */
	bootdesc = (struct bootdesc *)_end;

	/* Check if bootdesc is on an unmapped page */
	if (is_page_aligned(bootdesc))
		l4_map_helper(bootdesc - pager_offset, PAGE_SIZE);

	/* Allocate bootdesc sized structure */
	initdata->bootdesc = alloc_bootmem(bootdesc->desc_size, 0);

	/* Copy bootdesc to initdata */
	memcpy(initdata->bootdesc, bootdesc,
	       bootdesc->desc_size);

	if (npages > 0)
		l4_unmap_helper((void *)page_align_up(_end),
				PAGE_SIZE * npages);
}
