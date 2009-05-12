/*
 * Kernel Interface Page
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#ifndef __KIP_H__
#define __KIP_H__

/* FIXME: LICENCE: Taken from a forum post. Must reimplement with GPL terms */
#define __YEAR__ ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 \
			+ (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))

#define __MONTH__ (__DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 0 : 5) \
		: __DATE__ [2] == 'b' ? 1 \
		: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 2 : 3) \
		: __DATE__ [2] == 'y' ? 4 \
		: __DATE__ [2] == 'l' ? 6 \
		: __DATE__ [2] == 'g' ? 7 \
		: __DATE__ [2] == 'p' ? 8 \
		: __DATE__ [2] == 't' ? 9 \
		: __DATE__ [2] == 'v' ? 10 : 11)

#define __DAY__ ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10 \
		+ (__DATE__ [5] - '0'))

//#define DATE_AS_INT (((YEAR - 2000) * 12 + MONTH) * 31 + DAY)


struct kernel_desc {
	u16 rsrv;
	u8 subid;
	u8 id;
	u16 gendate;
	u16 date_rsrv;
	u16 subsubver;
	u8 subver;
	u8 ver;
	u32 supplier;
} __attribute__((__packed__));

/* Experimental KIP with non-standard offsets */
struct kip {
	/* System descriptions */
	u32 magic;
	u16 version_rsrv;
	u8  api_subversion;
	u8  api_version;
	u32 api_flags;

	u32 kmem_control;
	u32 time;

	u32 space_control;
	u32 thread_control;
	u32 ipc_control;
	u32 map;
	u32 ipc;
	u32 kread;
	u32 unmap;
	u32 exchange_registers;
	u32 thread_switch;
	u32 schedule;
	u32 getid;

	u32 arch_syscall0;
	u32 arch_syscall1;
	u32 arch_syscall2;
	u32 arch_syscall3;

	u32 utcb;

	struct kernel_desc kdesc;
} __attribute__((__packed__));

/*
 * Task id defs for priviledged server tasks. These are dynamically allocated
 * from the id pool, but still the pool should allocate them in this order.
 */
#define PAGER_TID			0
#define VFS_TID				1
#define BLKDEV_TID			2

#define __PAGERNAME__			"mm0"
#define __VFSNAME__			"fs0"
#define __BLKDEVNAME__			"blkdev0"

#define KDATA_PAGE_MAP			0
#define KDATA_BOOTDESC			1
#define KDATA_BOOTDESC_SIZE		2

#if defined (__KERNEL__)
extern struct kip kip;
#endif /* __KERNEL__ */


#endif /* __KIP_H__ */
