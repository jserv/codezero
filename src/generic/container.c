/*
 * Containers defined for current build.
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <l4/generic/container.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include INC_GLUE(memory.h)

/*
 * FIXME:
 * Add irqs, exceptions
 */

struct container_info cinfo[] = {
	[0] = {
	.name = "Codezero POSIX Services",
	.npagers = 1,
	.pager = {
		[0] = {
			.pager_lma = __pfn(0x38000),
			.pager_vma = __pfn(0xE0000000),
			.pager_size = __pfn(0x96000),
			.ncaps = 11,
			.caps = {
			[0] = {
				.type = CAP_TYPE_MAP | CAP_RTYPE_VIRTMEM,
				.access = CAP_MAP_READ | CAP_MAP_WRITE
					| CAP_MAP_EXEC | CAP_MAP_UNMAP,
				.access = 0,
				.start = __pfn(0xE0000000),
				.end = __pfn(0xF0000000),
				.size = __pfn(0x10000000),
			},
			[1] = {
				.type = CAP_TYPE_MAP | CAP_RTYPE_VIRTMEM,
				.access = CAP_MAP_READ | CAP_MAP_WRITE
					| CAP_MAP_EXEC | CAP_MAP_UNMAP,
				.start = __pfn(0x10000000),
				.end = __pfn(0x20000000),
				.size = __pfn(0x10000000),
			},
			[2] = {
				.type = CAP_TYPE_MAP | CAP_RTYPE_VIRTMEM,
				.access = CAP_MAP_READ | CAP_MAP_WRITE
					| CAP_MAP_EXEC | CAP_MAP_UNMAP,
				.access = 0,
				.start = __pfn(0x20000000),
				.end = __pfn(0x30000000),
				.size = __pfn(0x10000000),
			},
			[3] = {
				.type = CAP_TYPE_MAP | CAP_RTYPE_PHYSMEM,
				.access = CAP_MAP_CACHED | CAP_MAP_UNCACHED
					| CAP_MAP_READ | CAP_MAP_WRITE
					| CAP_MAP_EXEC | CAP_MAP_UNMAP,
				.start = __pfn(0x38000),
				.end = __pfn(0x1000000),	/* 16 MB for all posix services */
			},
			[4] = {
				.type = CAP_TYPE_IPC | CAP_RTYPE_CONTAINER,
				.access = CAP_IPC_SEND | CAP_IPC_RECV
					| CAP_IPC_FULL | CAP_IPC_SHORT
					| CAP_IPC_EXTENDED,
				.start = 0, .end = 0, .size = 0,
			},
			[5] = {
				.type = CAP_TYPE_TCTRL | CAP_RTYPE_CONTAINER,
				.access = CAP_TCTRL_CREATE | CAP_TCTRL_DESTROY
					| CAP_TCTRL_SUSPEND | CAP_TCTRL_RESUME
					| CAP_TCTRL_RECYCLE,
				.start = 0, .end = 0, .size = 0,
			},
			[6] = {
				.type = CAP_TYPE_EXREGS | CAP_RTYPE_CONTAINER,
				.access = CAP_EXREGS_RW_PAGER
					| CAP_EXREGS_RW_UTCB | CAP_EXREGS_RW_SP
					| CAP_EXREGS_RW_PC | CAP_EXREGS_RW_REGS,
				.start = 0, .end = 0, .size = 0,
			},
			[7] = {
				.type = CAP_TYPE_QUANTITY
					| CAP_RTYPE_THREADPOOL,
				.access = 0, .start = 0, .end = 0,
				.size = 64,
			},
			[8] = {
				.type = CAP_TYPE_QUANTITY | CAP_RTYPE_SPACEPOOL,
				.access = 0, .start = 0, .end = 0,
				.size = 64,
			},
			[9] = {
				.type = CAP_TYPE_QUANTITY | CAP_RTYPE_CPUPOOL,
				.access = 0, .start = 0, .end = 0,
				.size = 50,	/* Percentage */
			},
			[10] = {
				.type = CAP_TYPE_QUANTITY | CAP_RTYPE_MUTEXPOOL,
				.access = 0, .start = 0, .end = 0,
				.size = 100,
			},
			},
		},
	},
	},
};

