#ifndef __PLATFORM_IRQ_H__
#define __PLATFORM_IRQ_H__

#define IRQ_CHIPS_MAX				2

/*
 * Globally unique irq chip offsets:
 *
 * A global irq number is calculated as
 * chip_offset + local_irq_offset.
 *
 * This way, the global irq number uniquely represents
 * an irq on any irq chip.
 */
#define VIC_CHIP_OFFSET			0
#define SIC_CHIP_OFFSET			32

/* Maximum irqs on VIC and SIC */
#define VIC_IRQS_MAX			PL190_IRQS_MAX
#define SIC_IRQS_MAX			PL190_SIC_IRQS_MAX

#define IRQS_MAX			VIC_IRQS_MAX + SIC_IRQS_MAX

/* Vectored Interrupt Controller local IRQ numbers */
#define VIC_IRQ_TIMER01			4
#define VIC_IRQ_TIMER23			5
#define VIC_IRQ_RTC			10
#define VIC_IRQ_UART0			12
#define VIC_IRQ_UART1			13
#define VIC_IRQ_UART2			14
#define VIC_IRQ_SIC			31

/* Secondary Interrupt controller local IRQ numbers */
#define SIC_IRQ_SWI			0
#define SIC_IRQ_UART3			6

/* Global irq numbers */
#define IRQ_TIMER01		(VIC_IRQ_TIMER01 + VIC_CHIP_OFFSET)
#define IRQ_TIMER23		(VIC_IRQ_TIMER23 + VIC_CHIP_OFFSET)
#define IRQ_RTC			(VIC_IRQ_RTC + VIC_CHIP_OFFSET)
#define IRQ_UART0		(VIC_IRQ_UART0 + VIC_CHIP_OFFSET)
#define IRQ_UART1		(VIC_IRQ_UART1 + VIC_CHIP_OFFSET)
#define IRQ_UART2		(VIC_IRQ_UART2 + VIC_CHIP_OFFSET)
#define IRQ_SIC			(VIC_IRQ_SIC + VIC_CHIP_OFFSET)

#define IRQ_SICSWI		(SIC_IRQ_SWI + SIC_CHIP_OFFSET)
#define IRQ_UART3		(SIC_IRQ_UART3 + SIC_CHIP_OFFSET)


#endif /* __PLATFORM_IRQ_H__ */
