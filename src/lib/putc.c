/*
 * Generic putc implementation that ties with platform-specific uart driver.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#include INC_PLAT(uart.h)

void putc(char c)
{
	if (c == '\n')
		uart_putc('\r');
	uart_putc(c);
}
