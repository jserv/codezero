/*
 * Kernel Interface Page and sys_kdata_read()
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */

#include <l4/generic/tcb.h>
#include <l4/generic/physmem.h>
#include INC_API(kip.h)
#include INC_API(syscall.h)
#include INC_GLUE(memlayout.h)
#include INC_ARCH(bootdesc.h)

/* FIXME: Change the unit name */
UNIT("kip") struct kip kip;

/* Error-checked kernel data request call */
int __sys_kread(int rd, void *dest)
{
	int err = 0;

	switch(rd) {
	case KDATA_PAGE_MAP:
		/*
		 * FIXME:FIXME: Check if address is mapped here first!!!
		 * Also check if process has enough buffer for physmem to fit!!!
		 */
		printk("Handling KDATA_PAGE_MAP request.\n");
		memcpy(dest, &page_map, sizeof(page_map));
		break;
	case KDATA_BOOTDESC:
		printk("Handling KDATA_BOOTDESC request.\n");
		/*
		 * FIXME:FIXME: Check if address is mapped here first!!!
		 * Also check if process has enough buffer for physmem to fit!!!
		 */
		memcpy(dest, bootdesc, bootdesc->desc_size);
		break;
	case KDATA_BOOTDESC_SIZE:
		printk("Handling KDATA_BOOTDESC_SIZE request.\n");
		/*
		 * FIXME:FIXME: Check if address is mapped here first!!!
		 * Also check if process has enough buffer for physmem to fit!!!
		 */
		*(unsigned int *)dest = bootdesc->desc_size;
		break;

	default:
		printk("Unsupported kernel data request.\n");
		err = -1;
	}
	return err;

}

/*
 * Privilaged tasks use this call to request data about the system during their
 * initialisation. This read-like call is only available during system startup.
 * It is much more flexible to use this method rather than advertise a customly
 * forged KIP to all tasks throughout the system lifetime. Note, this does not
 * support file positions, any such features aren't supported since this is call
 * is discarded after startup.
 */
int sys_kread(struct syscall_args *a)
{
	unsigned int *arg = KTCB_REF_ARG0(current);
	void *addr = (void *)arg[1];	/* Buffer address */
	int rd = (int)arg[0];	/* Request descriptor */

	/* Error checking */
	if ((rd < 0) || (addr <= 0)) {
		printk("%s: Invalid arguments.\n", __FUNCTION__);
		return -1;
	}
	return __sys_kread(rd, addr);
}

