/*
 * Describes physical memory layout of EB platform.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#ifndef __PLATFORM_EB_OFFSETS_H__
#define __PLATFORM_EB_OFFSETS_H__

/* Physical memory base */
#define	PHYS_MEM_START			0x00000000 /* inclusive */
#define	PHYS_MEM_END			0x10000000 /* 256 MB, exclusive */

/*
 * These bases taken from where kernel is `physically' linked at,
 * also used to calculate virtual-to-physical translation offset.
 * See the linker script for their sources. PHYS_ADDR_BASE can't
 * use a linker variable because it's referred from assembler.
 */
#define	PHYS_ADDR_BASE			0x100000

/* Device memory base */
#define	EB_DEV_PHYS			0x10000000

/* Device offsets in physical memory */
#define	EB_SYSTEM_REGISTERS		0x10000000 /* System registers */
#define	EB_SYSCTRL_BASE			0x10001000 /* System controller */
#define EB_UART0_BASE           	0x10009000 /* UART 0 */
#define EB_UART1_BASE           	0x1000A000 /* UART 1 */
#define EB_UART2_BASE           	0x1000B000 /* UART 2 */
#define EB_UART3_BASE           	0x1000C000 /* UART 3 */
#define EB_WATCHDOG0_BASE       	0x10010000 /* WATCHDOG */
#define EB_TIMER01_BASE         	0x10011000 /* TIMER 0-1 */
#define EB_TIMER23_BASE         	0x10012000 /* TIMER 2-3 */
#define EB_RTC_BASE			0x10017000 /* RTC interface */
#define EB_GIC0_BASE            	0x10040000 /* GIC 0 */
#define EB_GIC1_BASE            	0x10050000 /* GIC 1 */
#define EB_GIC2_BASE            	0x10060000 /* GIC 2 */
#define EB_GIC3_BASE            	0x10070000 /* GIC 3 */

/*
 * Uart virtual address until a file-based console access
 * is available for userspace
 */
#define	USERSPACE_UART_BASE		0x500000

/*
 * Device offsets in virtual memory. They offset to some virtual
 * device base address. Each page on this virtual base is consecutively
 * allocated to devices. Nice and smooth.
 */
#define EB_SYSREGS_VOFFSET	0x00000000
#define EB_SYSCTRL_VOFFSET	0x00001000
#define EB_UART0_VOFFSET	0x00002000
#define EB_TIMER01_VOFFSET	0x00003000
#define EB_GIC0_VOFFSET		0x00004000
#define EB_GIC1_VOFFSET       	0x00005000
#define EB_GIC2_VOFFSET       	0x00006000
#define EB_GIC3_VOFFSET       	0x00007000

#define EB_SYSREGS_VBASE	(IO_AREA0_VADDR + EB_SYSREGS_VOFFSET)
#define EB_SYSCTRL_VBASE	(IO_AREA0_VADDR + EB_SYSCTRL0_VOFFSET)
#define EB_UART0_VBASE		(IO_AREA0_VADDR + EB_UART0_VOFFSET)
#define EB_TIMER01_VBASE	(IO_AREA0_VADDR + EB_TIMER01_VOFFSET)
#define EB_GIC0_VBASE         	(IO_AREA0_VADDR + EB_GIC0_VOFFSET)
#define EB_GIC1_VBASE         	(IO_AREA0_VADDR + EB_GIC1_VOFFSET)
#define EB_GIC2_VBASE         	(IO_AREA0_VADDR + EB_GIC2_VOFFSET)
#define EB_GIC3_VBASE         	(IO_AREA0_VADDR + EB_GIC3_VOFFSET)

#endif /* __PLATFORM_EB_OFFSETS_H__ */

