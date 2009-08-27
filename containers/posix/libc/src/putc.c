#include <arch/uart.h>

void putc(char c)
{
	if (c == '\n')
		uart_putc('\r');
	uart_putc(c);
}
