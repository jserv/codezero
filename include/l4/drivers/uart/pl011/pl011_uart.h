/*
 * PL011 UART Generic driver implementation.
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 * The particular intention of this code is that it has been carefully written
 * as decoupled from os-specific code and in a verbose way such that it clearly
 * demonstrates how the device operates, reducing the amount of time to be spent
 * for understanding the operational model and implementing a driver from
 * scratch. This is the very first to be such a driver so far, hopefully it will
 * turn out to be useful.
 */

#ifndef __PL011_UART_H__
#define __PL011_UART_H__

#include INC_PLAT(uart.h)
#include INC_ARCH(io.h)

/* Register offsets */
#define PL011_UARTDR		0x00
#define PL011_UARTRSR		0x04
#define PL011_UARTECR		0x04
#define PL011_UARTFR		0x18
#define PL011_UARTILPR		0x20
#define PL011_UARTIBRD		0x24
#define PL011_UARTFBRD		0x28
#define PL011_UARTLCR_H		0x2C
#define PL011_UARTCR		0x30
#define PL011_UARTIFLS		0x34
#define PL011_UARTIMSC		0x38
#define PL011_UARTRIS		0x3C
#define PL011_UARTMIS		0x40
#define PL011_UARTICR		0x44
#define PL011_UARTDMACR		0x48

/* IRQ bits for each uart irq event */
#define PL011_RXIRQ		(1 << 4)
#define PL011_TXIRQ		(1 << 5)
#define PL011_RXTIMEOUTIRQ	(1 << 6)
#define PL011_FEIRQ		(1 << 7)
#define PL011_PEIRQ		(1 << 8)
#define PL011_BEIRQ		(1 << 9)
#define PL011_OEIRQ		(1 << 10)

struct pl011_uart;

void pl011_initialise_driver();

int pl011_initialise_device(struct pl011_uart * uart);

int pl011_tx_char(unsigned int base, char c);
int pl011_rx_char(unsigned int base, char *c);

void pl011_set_baudrate(unsigned int base, unsigned int baud, \
							unsigned int clkrate);
void pl011_set_irq_mask(unsigned int base, unsigned int flags);
void pl011_clr_irq_mask(unsigned int base, unsigned int flags);

void pl011_irq_handler(struct pl011_uart * uart);
void pl011_tx_irq_handler(struct pl011_uart * uart, unsigned int flags);
void pl011_rx_irq_handler(struct pl011_uart *uart, unsigned int flags);
void pl011_error_irq_handler(struct pl011_uart *uart, unsigned int flags);

struct pl011_uart {
	unsigned int base;
	unsigned int frame_errors;
	unsigned int parity_errors;
	unsigned int break_errors;
	unsigned int overrun_errors;
	unsigned int rx_timeout_errors;
};

#define PL011_UARTEN	(1 << 0)
static inline void pl011_uart_enable(unsigned int uart_base)
{
	unsigned int val;
	val = 0;

	read(val, (uart_base + PL011_UARTCR));
	val |= PL011_UARTEN;
	write(val, (uart_base + PL011_UARTCR));
	
	return;
}

static inline void pl011_uart_disable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTCR));
	val &= ~PL011_UARTEN;
	write(val, (uart_base + PL011_UARTCR));

	return;
}

#define PL011_TXE	(1 << 8)
static inline void pl011_tx_enable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTCR));
	val |= PL011_TXE;
	write(val, (uart_base + PL011_UARTCR));
	return;
}

static inline void pl011_tx_disable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTCR));
	val &= ~PL011_TXE;
	write(val, (uart_base + PL011_UARTCR));
	return;
}


#define PL011_RXE	(1 << 9)
static inline void pl011_rx_enable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTCR));
	val |= PL011_RXE;
	write(val, (uart_base + PL011_UARTCR));
	return;
}

static inline void pl011_rx_disable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTCR));
	val &= ~PL011_RXE;
	write(val, (uart_base + PL011_UARTCR));
	return;
}

#define PL011_TWO_STOPBITS_SELECT	(1 << 3)
static inline void pl011_set_stopbits(unsigned int uart_base, int stopbits)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTLCR_H));

	if(stopbits == 2) {
		/* Set to two bits */
		val |= PL011_TWO_STOPBITS_SELECT;
	} else {
		/* Default is 1 */
		val &= ~PL011_TWO_STOPBITS_SELECT;
	}
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

#define PL011_PARITY_ENABLE	(1 << 1)
static inline void pl011_parity_enable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTLCR_H));
	val |= PL011_PARITY_ENABLE;
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

static inline void pl011_parity_disable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTLCR_H));
	val &= ~PL011_PARITY_ENABLE;
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

#define PL011_PARITY_EVEN	(1 << 2)
static inline void pl011_set_parity_even(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTLCR_H));
	val |= PL011_PARITY_EVEN;
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

static inline void pl011_set_parity_odd(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTLCR_H));
	val &= ~PL011_PARITY_EVEN;
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

#define	PL011_ENABLE_FIFOS	(1 << 4)
static inline void pl011_enable_fifos(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTLCR_H));
	val |= PL011_ENABLE_FIFOS;
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

static inline void pl011_disable_fifos(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTLCR_H));
	val &= ~PL011_ENABLE_FIFOS;
	write(val, (uart_base + PL011_UARTLCR_H));
	return;
}

#define PL011_WORD_WIDTH_SHIFT	(5)
/* Sets the transfer word width for the data register. */
static inline void pl011_set_word_width(unsigned int uart_base, int size)
{
	unsigned int val = 0;

	if(size < 5 || size > 8)	/* Default is 8 */
		size = 8;

	/* Clear size field */
	read(val, (uart_base + PL011_UARTLCR_H));
	val &= ~(0x3 << PL011_WORD_WIDTH_SHIFT);
	write(val, (uart_base + PL011_UARTLCR_H));

	/* The formula is to write 5 less of size given:
	 * 11 = 8 bits
	 * 10 = 7 bits
	 * 01 = 6 bits
	 * 00 = 5 bits
	 */
	read(val, (uart_base + PL011_UARTLCR_H));
	val |= (size - 5) << PL011_WORD_WIDTH_SHIFT;
	write(val, (uart_base + PL011_UARTLCR_H));

	return;
}

/*
 * Defines at which level of fifo fullness an irq will be generated.
 * @xfer: tx fifo = 0, rx fifo = 1
 * @level: Generate irq if:
 * 0	rxfifo >= 1/8 full  	txfifo <= 1/8 full
 * 1	rxfifo >= 1/4 full  	txfifo <= 1/4 full
 * 2	rxfifo >= 1/2 full  	txfifo <= 1/2 full
 * 3	rxfifo >= 3/4 full	txfifo <= 3/4 full
 * 4	rxfifo >= 7/8 full	txfifo <= 7/8 full
 * 5-7	reserved		reserved
 */
static inline void pl011_set_irq_fifolevel(unsigned int uart_base, \
										 unsigned int xfer, unsigned int level)
{
	if(xfer != 1 && xfer != 0)	/* Invalid fifo */
		return;
	if(level > 4)			/* Invalid level */
		return;
	write(level << (xfer * 3), (uart_base + PL011_UARTIFLS));
	return;
}

/* returns which irqs are masked */
static inline unsigned int pl011_read_irqmask(unsigned int uart_base)
{
	unsigned int flags;
	read(flags, (uart_base + PL011_UARTIMSC));
	return flags;
}

/* returns masked irq status */
static inline unsigned int pl011_read_irqstat(unsigned int uart_base)
{
	unsigned int irqstatus;
	read(irqstatus, (uart_base + PL011_UARTMIS));
	return irqstatus;
}
/* Clears the given asserted irqs */
static inline void pl011_irq_clear(unsigned int uart_base, unsigned int flags)
{
	if(flags > 0x3FF) {	/* Invalid irq clearing bitvector */
		return;
	}
	/* Simply write the flags since it's a write-only register */
	write(flags, (uart_base + PL011_UARTICR));
	return;
}

#define PL011_TXDMAEN	(1 << 1)
#define PL011_RXDMAEN	(1 << 0)

/* Enables dma transfers for uart. The dma controller
 * must be initialised, set-up and enabled separately.
 */
static inline void pl011_tx_dma_enable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTDMACR));
	val |= PL011_TXDMAEN;
	write(val, (uart_base + PL011_UARTDMACR));
	return;
}

/* Disables dma transfers for uart */
static inline void pl011_tx_dma_disable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTDMACR));
	val &= ~PL011_TXDMAEN;
	write(val, (uart_base + PL011_UARTDMACR));
	return;
}

static inline void pl011_rx_dma_enable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTDMACR));
	val |= PL011_RXDMAEN;
	write(val, (uart_base + PL011_UARTDMACR));
	return;
}

static inline void pl011_rx_dma_disable(unsigned int uart_base)
{
	unsigned int val = 0;

	read(val, (uart_base + PL011_UARTDMACR));
	val &= ~PL011_RXDMAEN;
	write(val, (uart_base + PL011_UARTDMACR));
	return;
}

#endif /* __PL011_UART__ */
