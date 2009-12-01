#ifndef __PB926_PLATFORM_H__
#define __PB926_PLATFORM_H__
/*
 * Platform specific ties between drivers and generic APIs used by the kernel.
 * E.g. system timer and console.
 *
 * Copyright (C) Bahadir Balban 2007
 */

#include INC_PLAT(offsets.h)
#include INC_GLUE(memlayout.h)
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/generic/resource.h>

/* Default console used by kernel */
#define	PLATFORM_CONSOLE_BASE		PB926_UART0_BASE

/* SP804 timer has TIMER1 at TIMER0 + 0x20 address */
#define	PLATFORM_TIMER0_BASE		PB926_TIMER01_BASE

/* Total number of timers present in this platform */
#define	TOTAL_TIMERS			4

#define PLATFORM_TIMER0	0
#define PLATFORM_TIMER1	1
#define PLATFORM_TIMER2	2
#define PLATFORM_TIMER3	3

#define PB926_UART_SIZE			0x1000
#define PB926_TIMER_SIZE		0x1000

#define	PLATFORM_UART1_BASE		PB926_UART1_BASE
#define	PLATFORM_UART2_BASE		PB926_UART2_BASE
#define	PLATFORM_UART3_BASE		PB926_UART3_BASE

#define	PLATFORM_UART1_SIZE		PB926_UART_SIZE
#define	PLATFORM_UART2_SIZE		PB926_UART_SIZE
#define	PLATFORM_UART3_SIZE		PB926_UART_SIZE
#define	PLATFORM_TIMER1_BASE		PB926_TIMER23_BASE
#define	PLATFORM_TIMER1_SIZE		PB926_TIMER_SIZE

int platform_setup_device_caps(struct kernel_resources *kres);
void platform_irq_enable(int irq);
void platform_irq_disable(int irq);
void timer_start(void);

#endif /* __PB926_PLATFORM_H__ */
