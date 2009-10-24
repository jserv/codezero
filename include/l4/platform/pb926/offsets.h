/*
 * Describes physical memory layout of pb926 platform.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#ifndef __PLATFORM_PB926_OFFSETS_H__
#define __PLATFORM_PB926_OFFSETS_H__

/* Physical memory base */
#define	PHYS_MEM_START			0x00000000 /* inclusive */
#define	PHYS_MEM_END			0x08000000 /* 128 MB, exclusive */

/*
 * These bases taken from where kernel is `physically' linked at,
 * also used to calculate virtual-to-physical translation offset.
 * See the linker script for their sources. PHYS_ADDR_BASE can't
 * use a linker variable because it's referred from assembler.
 */
#define	PHYS_ADDR_BASE			0x100000

/* Device memory base */
#define	PB926_DEV_PHYS			0x10000000

/* Device offsets in physical memory */
#define	PB926_SYSTEM_REGISTERS		0x10000000 /* System registers */
#define	PB926_SYSCTRL_BASE		0x101E0000 /* System controller */
#define	PB926_WATCHDOG_BASE		0x101E1000 /* Watchdog */
#define	PB926_TIMER01_BASE		0x101E2000 /* Timers 0 and 1 */
#define	PB926_TIMER23_BASE		0x101E3000 /* Timers 2 and 3 */
#define	PB926_RTC_BASE			0x101E8000 /* Real Time Clock */
#define	PB926_VIC_BASE			0x10140000 /* Primary Vectored IC */
#define	PB926_SIC_BASE			0x10003000 /* Secondary IC */
#define	PB926_UART0_BASE		0x101F1000 /* Console port (UART0) */
#define	PB926_UART1_BASE		0x101F2000 /* Console port (UART1) */
#define	PB926_UART2_BASE		0x101F3000 /* Console port (UART2) */
#define	PB926_UART3_BASE		0x10009000 /* Console port (UART3) */

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
#define PB926_TIMER01_VOFFSET			0x00000000
#define PB926_UART0_VOFFSET			0x00001000
#define	PB926_VIC_VOFFSET			0x00002000
#define	PB926_SIC_VOFFSET			0x00003000
#define PB926_SYSREGS_VOFFSET			0x00005000
#define PB926_SYSCTRL_VOFFSET			0x00006000

#define PB926_UART0_VBASE			(IO_AREA0_VADDR + PB926_UART0_VOFFSET)
#define PB926_TIMER01_VBASE			(IO_AREA0_VADDR + PB926_TIMER01_VOFFSET)
#define PB926_SYSCTRL_VBASE			(IO_AREA0_VADDR + PB926_SYSCTRL_VOFFSET)
#define PB926_VIC_VBASE				(IO_AREA0_VADDR + PB926_VIC_VOFFSET)
#define PB926_SIC_VBASE				(IO_AREA0_VADDR + PB926_SIC_VOFFSET)

#endif /* __PLATFORM_PB926_OFFSETS_H__ */

