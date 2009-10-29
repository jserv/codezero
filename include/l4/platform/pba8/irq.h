#ifndef __PLATFORM_IRQ_H__
#define __PLATFORM_IRQ_H__

/* TODO: Not sure about this, need to check */
#define IRQ_CHIPS_MAX			4
#define IRQS_MAX			96

/*
 * IRQ indices, 32-63 and 72-89 index
 * available for external sources
 * 0-32: used for SI, provided by
 * distributed interrupt controller
 */
#define IRQ_TIMER01			36
#define IRQ_TIMER23			37
#define IRQ_TIMER45                     73
#define IRQ_TIMER67                     74
#define IRQ_RTC				42
#define IRQ_UART0			44
#define IRQ_UART1			45
#define IRQ_UART2			46
#define IRQ_UART3                       47


#endif /* __PLATFORM_IRQ_H__ */
