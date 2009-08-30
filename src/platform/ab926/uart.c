/*
 * Ties up platform's uart driver functions with generic API
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/platform.h>
#include INC_PLAT(platform.h)

#include <l4/drivers/uart/pl011/pl011_uart.h>

extern struct pl011_uart uart;

void uart_init()
{
	uart.base = PL011_BASE;
	uart.ops.initialise(&uart);
}

/* Generic uart function that lib/putchar.c expects to see implemented */
void uart_putc(char c)
{
	int res;
	/* Platform specific uart implementation */
	do {
		res = uart.ops.tx_char(c);
	} while (res < 0);
}

