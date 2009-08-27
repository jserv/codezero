/*
 * Ties up platform's uart driver functions with generic API
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#include <arch/uart.h>
#include <arch/pl011_uart.h>

/* UART-specific internal error codes */
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

struct pl011_uart uart;

int pl011_tx_char(char c)
{
	unsigned int val;
	val = 0;
	
	read(val, PL011_UARTFR);
	if(val & PL011_TXFF) {		/* TX FIFO Full */
		return -PL011_EAGAIN;
	}
	write(c, PL011_UARTDR);
	return 0;
}

int pl011_rx_char(char * c)
{
	unsigned int data;
	unsigned int val;
	val = 0;
	
	read(val, PL011_UARTFR);
	if(val & PL011_RXFE) {		/* RX FIFO Empty */
		return -PL011_EAGAIN;
	}
	
	read(data, PL011_UARTDR);
	*c = (char) data;
	
	if((data >> 8) & 0xF) {		/* There were errors */
		return -1;		/* Signal error in xfer */
	}
	return 0;			/* No error return */
}


/* 
 * Sets the baud rate in kbps. It is recommended to use 
 * standard rates such as: 1200, 2400, 3600, 4800, 7200, 
 * 9600, 14400, 19200, 28800, 38400, 57600 76800, 115200.
 */
void pl011_set_baudrate(unsigned int baud, unsigned int clkrate)
{
	const unsigned int uartclk = 24000000;	/* 24Mhz clock fixed on pb926 */
	unsigned int val;
	unsigned int ipart, fpart;
	unsigned int remainder;

	remainder = 0;
	ipart = 0;
	fpart = 0;
	val   = 0;

	/* Use default pb926 rate if no rate is supplied */
	if(clkrate == 0) {
		clkrate = uartclk;
	}
	if(baud > 115200 || baud < 1200) {
		baud = 38400;	/* Default rate. */
	}
	/* 24000000 / (38400 * 16) */
	ipart = 39;

	write(ipart, PL011_UARTIBRD);
	write(fpart, PL011_UARTFBRD);

	/* For the IBAUD and FBAUD to update, we need to
	 * write to UARTLCR_H because the 3 registers are
	 * actually part of a single register in hardware
	 * which only updates by a write to UARTLCR_H */
	read(val, PL011_UARTLCR_H);
	write(val, PL011_UARTLCR_H);
	return;

}


/* Masks the irqs given in the flags bitvector. */
void pl011_set_irq_mask(unsigned int flags)
{
	unsigned int val;
	val = 0;
	
	if(flags > 0x3FF) {	/* Invalid irqmask bitvector */
		return;
	}
	
	read(val, PL011_UARTIMSC);
	val |= flags;
	write(val, PL011_UARTIMSC);
	return;
}


/* Clears the irqs given in flags from masking */
void pl011_clr_irq_mask(unsigned int flags)
{
	unsigned int val;
	val = 0;
	
	if(flags > 0x3FF) {	/* Invalid irqmask bitvector */
		return;
	}
	
	read(val, PL011_UARTIMSC);
	val &= ~flags;
	write(val, PL011_UARTIMSC);
	return;
}

int pl011_initialise(struct pl011_uart * uart)
{

	uart->frame_errors   = 0;
	uart->parity_errors  = 0;
	uart->break_errors   = 0;
	uart->overrun_errors = 0;

	/* Initialise data register for 8 bit data read/writes */
	pl011_set_word_width(8);

	/* Fifos are disabled because by default it is assumed the port
	 * will be used as a user terminal, and in that case the typed
	 * characters will only show up when fifos are flushed, rather than
	 * when each character is typed. We avoid this by not using fifos.
	 */
	pl011_disable_fifos();

	/* Set default baud rate of 38400 */
	pl011_set_baudrate(38400, 24000000);

	/* Set default settings of 1 stop bit, no parity, no hw flow ctrl */
	pl011_set_stopbits(1);
	pl011_parity_disable();

	/* Disable all irqs */
	pl011_set_irq_mask(0x3FF);

	/* Enable rx, tx, and uart chip */
	pl011_tx_enable();
	pl011_rx_enable();
	pl011_uart_enable();

	return 0;
}

void uart_init()
{
	uart.base = PL011_BASE;
	pl011_initialise(&uart);
}

/* Generic uart function that lib/putchar.c expects to see implemented */
void uart_putc(char c)
{
	int res;
	/* Platform specific uart implementation */
	do {
		res = pl011_tx_char(c);
	} while (res < 0);
}

