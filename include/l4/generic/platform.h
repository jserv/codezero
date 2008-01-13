#ifndef __PLATFORM_H__
#define __PLATFORM_H__
/*
 * Generic functions to be provided by every platform.
 */

void platform_init(void);

/* Uart APIs */
void uart_init(void);
void uart_putc(char c);

/* Timer APIs */
void timer_init(void);
void timer_start(void);

/* IRQ controller */
void irq_controller_init(void);
void platform_irq_enable(int irq);
void platform_irq_disable(int irq);

#endif /* __PLATFORM_H__ */
