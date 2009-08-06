/*
 * Capability manipulation syscall.
 *
 * The heart of Codezero security
 * mechanisms lay here.
 *
 * Copyright (C) 2009 Bahadir Balban
 */

#include <l4/api/capability.h>
#include <l4/generic/tcb.h>
#include <l4/generic/physmem.h>
#include <l4/generic/space.h>
#include <l4/api/errno.h>
#include INC_API(syscall.h)



/* Error-checked kernel data request call */
int __sys_capability_control(unsigned int req, unsigned int flags, void *userbuf)
{
	int err = 0;
#if 0
	switch(req) {
	case KDATA_PAGE_MAP:
		// printk("Handling KDATA_PAGE_MAP request.\n");
		if (check_access(vaddr, sizeof(page_map), MAP_USR_RW_FLAGS, 1) < 0)
			return -EINVAL;
		memcpy(dest, &page_map, sizeof(page_map));
		break;
	case KDATA_BOOTDESC:
		// printk("Handling KDATA_BOOTDESC request.\n");
		if (check_access(vaddr, bootdesc->desc_size, MAP_USR_RW_FLAGS, 1) < 0)
			return -EINVAL;
		memcpy(dest, bootdesc, bootdesc->desc_size);
		break;
	case KDATA_BOOTDESC_SIZE:
		// printk("Handling KDATA_BOOTDESC_SIZE request.\n");
		if (check_access(vaddr, sizeof(unsigned int), MAP_USR_RW_FLAGS, 1) < 0)
			return -EINVAL;
		*(unsigned int *)dest = bootdesc->desc_size;
		break;

	default:
		printk("Unsupported kernel data request.\n");
		err = -1;
	}
#endif
	return err;

}

int sys_capability_control(unsigned int req, unsigned int flags, void *userbuf)
{
	return __sys_capability_control(req, flags, userbuf);
}

