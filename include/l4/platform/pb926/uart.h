/*
 * Platform specific ties to generic uart functions that putc expects.
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */

#ifndef __PLATFORM_PB926_UART_H__
#define __PLATFORM_PB926_UART_H__

#include INC_PLAT(offsets.h)
#include INC_GLUE(memlayout.h)

#include <l4/drivers/uart/pl011/pl011_uart.h>

void uart_init(void);
void uart_putc(char c);

#endif /* __PLATFORM_PB926_UART_H__ */
