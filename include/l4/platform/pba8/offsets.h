/*
 * Describes physical memory layout of pb926 platform.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#ifndef __PLATFORM_PBA8_OFFSETS_H__
#define __PLATFORM_PBA8_OFFSETS_H__

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
#define	PBA8_DEV_PHYS			0x10000000

/* Device offsets in physical memory */
#define	PBA8_SYSTEM_REGISTERS		0x10000000 /* System registers */
#define	PBA8_SYSCTRL0_BASE		0x10001000 /* System controller 0 */
#define PBA8_UART0_BASE           	0x10009000 /* UART 0 */
#define PBA8_UART1_BASE           	0x1000A000 /* UART 1 */
#define PBA8_UART2_BASE           	0x1000B000 /* UART 2 */
#define PBA8_UART3_BASE           	0x1000C000 /* UART 3 */
#define PBA8_WATCHDOG0_BASE       	0x1000F000 /* WATCHDOG 0 */
#define PBA8_WATCHDOG1_BASE       	0x10010000 /* WATCHDOG 1 */
#define PBA8_TIMER01_BASE         	0x10011000 /* TIMER 0-1 */
#define PBA8_TIMER23_BASE         	0x10012000 /* TIMER 2-3 */
#define PBA8_RTC_BASE			0x10017000 /* RTC interface */
#define PBA8_TIMER45_BASE         	0x10018000 /* TIMER 4-5 */
#define PBA8_TIMER67_BASE         	0x10019000 /* TIMER 6-7 */
#define PBA8_SYSCTRL1_BASE        	0x1001A000 /* System controller 1 */
#define PBA8_GIC0_BASE            	0x1E000000 /* GIC 0 */
#define PBA8_GIC1_BASE            	0x1E010000 /* GIC 1 */
#define PBA8_GIC2_BASE            	0x1E020000 /* GIC 2 */
#define PBA8_GIC3_BASE            	0x1E030000 /* GIC 3 */

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
#define PBA8_SYSREGS_VOFFSET	0x00000000
#define PBA8_SYSCTRL0_VOFFSET	0x00001000
#define PBA8_SYSCTRL1_VOFFSET	0x00002000
#define PBA8_UART0_VOFFSET	0x00003000
#define PBA8_TIMER01_VOFFSET	0x00004000
#define PBA8_GIC0_VOFFSET	0x00005000
#define PBA8_GIC1_VOFFSET       0x00006000
#define PBA8_GIC2_VOFFSET       0x00007000
#define PBA8_GIC3_VOFFSET       0x00008000

#define PBA8_SYSREGS_VBASE	(IO_AREA0_VADDR + PBA8_SYSREGS_VOFFSET)
#define PBA8_SYSCTRL0_VBASE	(IO_AREA0_VADDR + PBA8_SYSCTRL0_VOFFSET)
#define PBA8_SYSCTRL1_VBASE     (IO_AREA0_VADDR + PBA8_SYSCTRL1_VOFFSET)
#define PBA8_UART0_VBASE	(IO_AREA0_VADDR + PBA8_UART0_VOFFSET)
#define PBA8_TIMER01_VBASE	(IO_AREA0_VADDR + PBA8_TIMER01_VOFFSET)
#define PBA8_GIC0_VBASE         (IO_AREA0_VADDR + PBA8_GIC0_VOFFSET)
#define PBA8_GIC1_VBASE         (IO_AREA0_VADDR + PBA8_GIC1_VOFFSET)
#define PBA8_GIC2_VBASE         (IO_AREA0_VADDR + PBA8_GIC2_VOFFSET)
#define PBA8_GIC3_VBASE         (IO_AREA0_VADDR + PBA8_GIC3_VOFFSET)

#endif /* __PLATFORM_PBA8_OFFSETS_H__ */

