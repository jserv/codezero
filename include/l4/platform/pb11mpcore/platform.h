#ifndef __PB11MPCORE_PLATFORM_H__
#define __PB11MPCORE_PLATFORM_H__
/*
 * Platform specific ties between drivers and generic APIs used by the kernel.
 * E.g. system timer and console.
 *
 * Copyright (C) Bahadir Balban 2007
 */

#include INC_PLAT(offsets.h)
#include INC_GLUE(memlayout.h)

#define PLATFORM_CONSOLE0_BASE			PB11MPCORE_UART0_VBASE
#define PLATFORM_TIMER0_BASE			PB11MPCORE_TIMER01_VBASE
/* Need to add syscntrl1 here */
#define PLATFORM_SP810_BASE			PB11MPCORE_SYSCTRL0_VBASE

/* Total number of timers present in this platform */
#define	TOTAL_TIMERS			8

#define PLATFORM_TIMER0	0
#define PLATFORM_TIMER1	1
#define PLATFORM_TIMER2	2
#define PLATFORM_TIMER3	3
#define PLATFORM_TIMER4	4
#define PLATFORM_TIMER5	5
#define PLATFORM_TIMER6	6
#define PLATFORM_TIMER7	7

void platform_irq_enable(int irq);
void platform_irq_disable(int irq);
void timer_start(void);

#endif /* __PB11MPCORE_PLATFORM_H__ */
