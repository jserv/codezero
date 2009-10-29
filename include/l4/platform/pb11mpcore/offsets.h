/*
 * Describes physical memory layout of PB11MPCORE platform.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#ifndef __PLATFORM_PB11MPCORE_OFFSETS_H__
#define __PLATFORM_PB11MPCORE_OFFSETS_H__

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
#define	PB11MPCORE_DEV_PHYS		0x10000000

/* Device offsets in physical memory */
#define	PB11MPCORE_SYSTEM_REGISTERS	0x10000000 /* System registers */
#define	PB11MPCORE_SYSCTRL0_BASE	0x10001000 /* System controller 0 */
#define PB11MPCORE_UART0_BASE           0x10009000 /* UART 0 */
#define PB11MPCORE_UART1_BASE           0x1000A000 /* UART 1 */
#define PB11MPCORE_UART2_BASE           0x1000B000 /* UART 2 */
#define PB11MPCORE_UART3_BASE           0x1000C000 /* UART 3 */
#define PB11MPCORE_WATCHDOG0_BASE       0x1000F000 /* WATCHDOG 0 */
#define PB11MPCORE_WATCHDOG1_BASE       0x10010000 /* WATCHDOG 1 */
#define PB11MPCORE_TIMER01_BASE         0x10011000 /* TIMER 0-1 */
#define PB11MPCORE_TIMER23_BASE         0x10012000 /* TIMER 2-3 */
#define PB11MPCORE_RTC_BASE		0x10017000 /* RTC interface */
#define PB11MPCORE_TIMER45_BASE         0x10018000 /* TIMER 4-5 */
#define PB11MPCORE_TIMER67_BASE         0x10019000 /* TIMER 6-7 */
#define PB11MPCORE_SYSCTRL1_BASE        0x1001A000 /* System controller 1 */
#define PB11MPCORE_GIC0_BASE            0x1E000000 /* GIC 0 */
#define PB11MPCORE_GIC1_BASE            0x1E010000 /* GIC 1 */
#define PB11MPCORE_GIC2_BASE            0x1E020000 /* GIC 2 */
#define PB11MPCORE_GIC3_BASE            0x1E030000 /* GIC 3 */

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
#define PB11MPCORE_SYSREGS_VOFFSET	0x00000000
#define PB11MPCORE_SYSCTRL0_VOFFSET	0x00001000
#define PB11MPCORE_SYSCTRL1_VOFFSET	0x00002000
#define PB11MPCORE_UART0_VOFFSET	0x00003000
#define PB11MPCORE_TIMER01_VOFFSET	0x00004000

#define PB11MPCORE_GIC0_VOFFSET		0x00005000
#define PB11MPCORE_GIC1_VOFFSET         0x00006000
#define PB11MPCORE_GIC2_VOFFSET         0x00007000
#define PB11MPCORE_GIC3_VOFFSET         0x00008000
#define PB11MPCORE_SYSREGS_VBASE	(IO_AREA0_VADDR + PB11MPCORE_SYSREGS_VOFFSET)
#define PB11MPCORE_SYSCTRL0_VBASE	(IO_AREA0_VADDR + PB11MPCORE_SYSCTRL0_VOFFSET)
#define PB11MPCORE_SYSCTRL1_VBASE       (IO_AREA0_VADDR + PB11MPCORE_SYSCTRL1_VOFFSET)
#define PB11MPCORE_UART0_VBASE		(IO_AREA0_VADDR + PB11MPCORE_UART0_VOFFSET)
#define PB11MPCORE_TIMER01_VBASE	(IO_AREA0_VADDR + PB11MPCORE_TIMER01_VOFFSET)
#define PB11MPCORE_GIC0_VBASE           (IO_AREA0_VADDR + PB11MPCORE_GIC0_VOFFSET)
#define PB11MPCORE_GIC1_VBASE           (IO_AREA0_VADDR + PB11MPCORE_GIC1_VOFFSET)
#define PB11MPCORE_GIC2_VBASE           (IO_AREA0_VADDR + PB11MPCORE_GIC2_VOFFSET)
#define PB11MPCORE_GIC3_VBASE           (IO_AREA0_VADDR + PB11MPCORE_GIC3_VOFFSET)

#endif /* __PLATFORM_PB11MPCORE_OFFSETS_H__ */

