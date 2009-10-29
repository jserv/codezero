#ifndef __PLATFORM_IRQ_H__
#define __PLATFORM_IRQ_H__

/* TODO: Not sure about this, need to check */
#define IRQ_CHIPS_MAX			4
#define IRQS_MAX			96

/*
 * IRQ indices,
 * GIC 0 and 1 are for logic tile 1
 * GIC 2 and 3 are for logic tile 2
 */
#define IRQ_TIMER01			4
#define IRQ_TIMER23			5
#define IRQ_RTC				10
#define IRQ_UART0			12
#define IRQ_UART1			13
#define IRQ_UART2			14
#define IRQ_UART3                       15

/*
 * TODO: Seems like GIC0 and GIC1 are cascaded for logic tile1
 * and GIC2 and GIC3 are cascaded for logic tile 2.
 * Interrupt Distribution:
 * 0-31: Used as SI provided by distributed interrupt controller
 * 32-63: Externel Peripheral Interrupts
 * 64-71: Interrupts from tile site 1
 * 72-79: Interrupts from tile site 2
 * 80-95: PCI and reserved Interrupts
 */
#endif /* __PLATFORM_IRQ_H__ */
