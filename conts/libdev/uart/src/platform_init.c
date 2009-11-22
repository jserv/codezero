/*
 * User space uart driver.
 *
 * Copyright (C) 2009, B Labs Ltd.
 */

#include <pl011_uart.h>

/*
 * Every task who wants to use this uart needs
 * to initialize an instance of this
 */
struct pl011_uart uart;

/*
 * Initialises the uart class data structures, and the device.
 * Terminal-like operation is assumed for default settings.
 */
int pl011_initialise(struct pl011_uart *uart)
{
	uart->frame_errors   = 0;
	uart->parity_errors  = 0;
	uart->break_errors   = 0;
	uart->overrun_errors = 0;

	/* Initialise data register for 8 bit data read/writes */
	pl011_set_word_width(uart->base, 8);

	/*
	 * Fifos are disabled because by default it is assumed the port
	 * will be used as a user terminal, and in that case the typed
	 * characters will only show up when fifos are flushed, rather than
	 * when each character is typed. We avoid this by not using fifos.
	 */
	pl011_disable_fifos(uart->base);

	/* Set default baud rate of 38400 */
	pl011_set_baudrate(uart->base, 38400, 24000000);

	/* Set default settings of 1 stop bit, no parity, no hw flow ctrl */
	pl011_set_stopbits(uart->base, 1);
	pl011_parity_disable(uart->base);

	/* Disable all irqs */
	pl011_set_irq_mask(uart->base, 0x3FF);

	/* Enable rx, tx, and uart chip */
	pl011_tx_enable(uart->base);
	pl011_rx_enable(uart->base);
	pl011_uart_enable(uart->base);

	return 0;
}

void platform_init(void)
{
	uart.base = PL011_BASE;
	pl011_initialise(&uart);
}

