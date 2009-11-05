/*
 * Ties up platform's uart driver functions with printf
 *
 * Copyright (C) 2009 B Labs Ltd.
 *
 */
#include <stdio.h>
#include <pl011_uart.h>

extern struct pl011_uart uart;

int __fputc(int c, FILE *stream)
{
	int res;
	do {
		res = pl011_tx_char(uart.base, c);
	} while( res < 0);

	return(0);
}

