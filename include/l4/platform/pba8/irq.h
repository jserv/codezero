#ifndef __PLATFORM_IRQ_H__
#define __PLATFORM_IRQ_H__

#define IRQ_CHIPS_MAX				2
#define IRQS_MAX				64

/* IRQ indices. */
#define IRQ_TIMER01			4
#define IRQ_TIMER23			5
#define IRQ_RTC				10
#define IRQ_UART0			12
#define IRQ_UART1			13
#define IRQ_UART2			14
#define IRQ_SIC				31

/* Cascading definitions */

#define PIC_IRQS_MAX			31 /* Total irqs on PIC */
/* The local irq line of the dummy peripheral on this chip */
#define LOCALIRQ_DUMMY			15
/* The irq index offset of this chip, is the maximum of previous chip + 1 */
#define SIRQ_CHIP_OFFSET		(PIC_IRQS_MAX + 1)
/* The global irq number of dummy is the local irq line + it's chip offset */
#define IRQ_DUMMY			(LOCALIRQ_DUMMY + SIRQ_CHIP_OFFSET)


#endif /* __PLATFORM_IRQ_H__ */
