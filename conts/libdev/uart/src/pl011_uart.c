/*
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <pl011_uart.h>

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

int pl011_tx_char(unsigned int base, char c)
{
	unsigned int val = 0;

	read(val, (base + PL011_UARTFR));
	if(val & PL011_TXFF) {		/* TX FIFO Full */
		return -PL011_EAGAIN;
	}
	write(c, (base + PL011_UARTDR));
	return 0;
}

int pl011_rx_char(unsigned int base, char * c)
{
	unsigned int data;
	unsigned int val = 0;

	read(val, (base + PL011_UARTFR));
	if(val & PL011_RXFE) {		/* RX FIFO Empty */
		return -PL011_EAGAIN;
	}

	read(data, (base + PL011_UARTDR));
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
void pl011_set_baudrate(unsigned int base, unsigned int baud,
			unsigned int clkrate)
{
	const unsigned int uartclk = 24000000;	/* 24Mhz clock fixed on pb926 */
	unsigned int val = 0, ipart = 0, fpart = 0;

	/* Use default pb926 rate if no rate is supplied */
	if(clkrate == 0) {
		clkrate = uartclk;
	}
	if(baud > 115200 || baud < 1200) {
		baud = 38400;	/* Default rate. */
	}
	/* 24000000 / (38400 * 16) */
	ipart = 39;

	write(ipart, (base + PL011_UARTIBRD));
	write(fpart, (base + PL011_UARTFBRD));

	/*
	 * For the IBAUD and FBAUD to update, we need to
	 * write to UARTLCR_H because the 3 registers are
	 * actually part of a single register in hardware
	 * which only updates by a write to UARTLCR_H
	 */
	read(val, (base + PL011_UARTLCR_H));
	write(val, (base + PL011_UARTLCR_H));
	return;

}

/* Masks the irqs given in the flags bitvector. */
void pl011_set_irq_mask(unsigned int base, unsigned int flags)
{
	unsigned int val = 0;

	if(flags > 0x3FF) {	/* Invalid irqmask bitvector */
		return;
	}

	read(val, (base + PL011_UARTIMSC));
	val |= flags;
	write(val, (base + PL011_UARTIMSC));
	return;
}

/* Clears the irqs given in flags from masking */
void pl011_clr_irq_mask(unsigned int base, unsigned int flags)
{
	unsigned int val = 0;

	if(flags > 0x3FF) {
		/* Invalid irqmask bitvector */
		return;
	}

	read(val, (base + PL011_UARTIMSC));
	val &= ~flags;
	write(val, (base + PL011_UARTIMSC));
	return;
}

