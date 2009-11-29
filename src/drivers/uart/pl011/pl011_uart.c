/*
 * PL011 Primecell UART driver
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#include <l4/drivers/uart/pl011/pl011_uart.h>
#include <l4/lib/bit.h>
#include INC_PLAT(platform.h)

struct pl011_uart uart;

/* UART-specific internal error codes.
 * TODO: Replace them when generic error codes are in place */
#define PL011_ERROR		1
#define PL011_EAGAIN		2

/* Error status bits in receive status register */
#define PL011_FE		(1 << 0)
#define PL011_PE		(1 << 1)
#define PL011_BE		(1 << 2)
#define PL011_OE		(1 << 3)

/* Status bits in flag register */
#define PL011_TXFE		(1 << 7)
#define PL011_RXFF		(1 << 6)
#define PL011_TXFF		(1 << 5)
#define PL011_RXFE		(1 << 4)
#define PL011_BUSY		(1 << 3)
#define PL011_DCD		(1 << 2)
#define PL011_DSR		(1 << 1)
#define PL011_CTS		(1 << 0)

int pl011_tx_char(unsigned int uart_base, char c)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTFR));

	if(val & PL011_TXFF) {
		/* TX FIFO Full */
		return -PL011_EAGAIN;
	}

	write(c, (uart_base + PL011_UARTDR));
	return 0;
}

int pl011_rx_char(unsigned int uart_base, char * c)
{
	unsigned int data;
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTFR));
	if(val & PL011_RXFE) {
		/* RX FIFO Empty */
		return -PL011_EAGAIN;
	}

	read(data, (uart_base + PL011_UARTDR));
	*c = (char) data;

	if((data >> 8) & 0xF) {
		/* There were errors, signal error */
		return -1;
	}
	return 0;
}

/*
 * Sets the baud rate in kbps. It is recommended to use
 * standard rates such as: 1200, 2400, 3600, 4800, 7200,
 * 9600, 14400, 19200, 28800, 38400, 57600 76800, 115200.
 */
void pl011_set_baudrate(unsigned int uart_base, unsigned int baud, \
							unsigned int clkrate)
{
	const unsigned int uartclk = 24000000;	/* 24Mhz clock fixed on pb926 */
	unsigned int val = 0;
	unsigned int ipart = 0, fpart = 0;

	/* Use default pb926 rate if no rate is supplied */
	if(clkrate == 0)
		clkrate = uartclk;
	if(baud > 115200 || baud < 1200)
		baud = 38400;	/* Default rate. */

	/* 24000000 / (16 * 38400) */
	ipart = 39;

	write(ipart, (uart_base + PL011_UARTIBRD));
	write(fpart, (uart_base + PL011_UARTFBRD));

	/*
	 * For the IBAUD and FBAUD to update, we need to
	 * write to UARTLCR_H because the 3 registers are
	 * actually part of a single register in hardware
	 * which only updates by a write to UARTLCR_H
	 */
	read(val, (uart_base + PL011_UARTLCR_H));
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

/* Masks the irqs given in the flags bitvector. */
void pl011_set_irq_mask(unsigned int uart_base, unsigned int flags)
{
	unsigned int val = 0;

	if(flags > 0x3FF) {
		/* Invalid irqmask bitvector */
		return;
	}

	read(val, (uart_base + PL011_UARTIMSC));
	val |= flags;
	write(val, (uart_base + PL011_UARTIMSC));
	return;
}

/* Clears the irqs given in flags from masking */
void pl011_clr_irq_mask(unsigned int uart_base, unsigned int flags)
{
	unsigned int val = 0;

	if(flags > 0x3FF) {
		/* Invalid irqmask bitvector */
		return;
	}

	read(val, (uart_base + PL011_UARTIMSC));
	val &= ~flags;
	write(val, (uart_base + PL011_UARTIMSC));
	return;
}

/*
 * Produces 1 character from data register and appends it into
 * rx buffer keeps record of timeout errors if one occurs.
 */
void pl011_rx_irq_handler(struct pl011_uart * uart, unsigned int flags)
{
	/*
	 * Currently we do nothing for uart irqs, because there's no external
	 * client to send/receive data (e.g. userspace processes kernel threads).
	 */
	return;
}

/* Consumes 1 character from tx buffer and attempts to transmit it */
void pl011_tx_irq_handler(struct pl011_uart * uart, unsigned int flags)
{
	/*
	 * Currently we do nothing for uart irqs, because there's no external
	 * client to send/receive data (e.g. userspace processes kernel threads).
	 */
	return;
}

/* Updates error counts and exits. Does nothing to recover errors */
void pl011_error_irq_handler(struct pl011_uart * uart, unsigned int flags)
{
	if(flags & PL011_FEIRQ) {
		uart->frame_errors++;
	}
	if(flags & PL011_PEIRQ) {
		uart->parity_errors++;
	}
	if(flags & PL011_BEIRQ) {
		uart->break_errors++;
	}
	if(flags & PL011_OEIRQ) {
		uart->overrun_errors++;
	}
	return;
}

void (* pl011_handlers[]) (struct pl011_uart *, unsigned int)  = {
	0,				/* Modem RI  */
	0,				/* Modem CTS */
	0,				/* Modem DCD */
	0,				/* Modem DSR */
	&pl011_rx_irq_handler,		/* Rx */
	&pl011_tx_irq_handler,		/* Tx */
	&pl011_rx_irq_handler,		/* Rx timeout */
	&pl011_error_irq_handler,	/* Framing error */
	&pl011_error_irq_handler,	/* Parity error  */
	&pl011_error_irq_handler,	/* Break error   */
	&pl011_error_irq_handler	/* Overrun error */
};

/* UART main entry for irq handling. It redirects actual
 * handling to handlers relevant to the irq that has occured.
 */
void pl011_irq_handler(struct pl011_uart * uart)
{
	unsigned int val;
	int handler_index;
	void (* handler)(struct pl011_uart *, unsigned int);

	val = pl011_read_irqstat(uart->base);

	handler_index = 32 - __clz(val);
	if(!handler_index) {	/* No irq */
		return;
	}
	/* Jump to right handler */
	handler = (void (*) (struct pl011_uart *, unsigned int))
			pl011_handlers[handler_index];

	/* If a handler is available, call it */
	if(handler)
		(*handler)(uart, val);

	return;
}

void pl011_initialise_driver(void)
{
	pl011_initialise_device(&uart);
}

/* Initialises the uart class data structures, and the device.
 * Terminal-like operation is assumed for default settings.
 */
int pl011_initialise_device(struct pl011_uart * uart)
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

	/* Install the irq handler */
	/* TODO: INSTALL IT HERE */

	/* Enable all irqs */
	pl011_clr_irq_mask(uart->base, 0x3FF);

	/* Enable rx, tx, and uart chip */
	pl011_tx_enable(uart->base);
	pl011_rx_enable(uart->base);
	pl011_uart_enable(uart->base);

	return 0;
}

