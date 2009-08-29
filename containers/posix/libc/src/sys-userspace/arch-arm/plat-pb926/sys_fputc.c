/*
 * Australian Public Licence B (OZPLB)
 * 
 * Version 1-0
 * 
 * Copyright (c) 2004 National ICT Australia
 * 
 * All rights reserved. 
 * 
 * Developed by: Embedded, Real-time and Operating Systems Program (ERTOS)
 *               National ICT Australia
 *               http://www.ertos.nicta.com.au
 * 
 * Permission is granted by National ICT Australia, free of charge, to
 * any person obtaining a copy of this software and any associated
 * documentation files (the "Software") to deal with the Software without
 * restriction, including (without limitation) the rights to use, copy,
 * modify, adapt, merge, publish, distribute, communicate to the public,
 * sublicense, and/or sell, lend or rent out copies of the Software, and
 * to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimers.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimers in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of National ICT Australia, nor the names of its
 *       contributors, may be used to endorse or promote products derived
 *       from this Software without specific prior written permission.
 * 
 * EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT
 * PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND
 * NATIONAL ICT AUSTRALIA AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS,
 * WARRANTIES OR CONDITIONS OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO ANY REPRESENTATIONS, WARRANTIES OR CONDITIONS
 * REGARDING THE CONTENTS OR ACCURACY OF THE SOFTWARE, OR OF TITLE,
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,
 * THE ABSENCE OF LATENT OR OTHER DEFECTS, OR THE PRESENCE OR ABSENCE OF
 * ERRORS, WHETHER OR NOT DISCOVERABLE.
 * 
 * TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
 * NATIONAL ICT AUSTRALIA OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL
 * THEORY (INCLUDING, WITHOUT LIMITATION, IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHERWISE) FOR ANY CLAIM, LOSS, DAMAGES OR OTHER
 * LIABILITY, INCLUDING (WITHOUT LIMITATION) LOSS OF PRODUCTION OR
 * OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA OR RECORDS; OR LOSS
 * OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT OR GOODWILL, OR
 * OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL, INDIRECT,
 * CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF OR IN
 * CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS WITH THE SOFTWARE, EVEN IF NATIONAL ICT AUSTRALIA OR ITS
 * CONTRIBUTORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS,
 * DAMAGES OR OTHER LIABILITY.
 * 
 * If applicable legislation implies representations, warranties, or
 * conditions, or imposes obligations or liability on National ICT
 * Australia or one of its contributors in respect of the Software that
 * cannot be wholly or partly excluded, restricted or modified, the
 * liability of National ICT Australia or the contributor is limited, to
 * the full extent permitted by the applicable legislation, at its
 * option, to:
 * a.  in the case of goods, any one or more of the following:
 * i.  the replacement of the goods or the supply of equivalent goods;
 * ii.  the repair of the goods;
 * iii. the payment of the cost of replacing the goods or of acquiring
 *  equivalent goods;
 * iv.  the payment of the cost of having the goods repaired; or
 * b.  in the case of services:
 * i.  the supplying of the services again; or
 * ii.  the payment of the cost of having the services supplied again.
 * 
 * The construction, validity and performance of this licence is governed
 * by the laws in force in New South Wales, Australia.
 */
#define MACHINE_PB926

#include <stdio.h>
#include <stdint.h>
#include <arch/pl011_uart.h>

//#define iPAQ	/* FIXME: this is ugly */
//#undef  XSCALE
//#define XSCALE
// #undef  iPAQ
// #define XSCALE

extern int __fputc(int c, FILE *stream);
/* Put character for elf-loader */
int
__fputc(int c, FILE *stream)
{

/* ---------------------------------- iPAQ & PLEB1 (SA-1100)--------------------------- */
#ifdef MACHINE_IPAQ_H3800 //iPAQ // SA-1100
	volatile char *base  = (char *)0x80050000;	// base serial interface address
	/* volatile int *base2 = (int *)0x80030000; // the other serial Arm serial i/f */

/*
UTSR1 32 @ 0x20:
 tby <0>    # Transmitter busy
 rne <1>    # Refeive FIFO not empty
 tnf <2>    # Transmitter not full
 pre <3>    # Parity error
 fre <4>    # Framing error
 ror <5>    # Receive FIFO overrun
*/

#define UTDR		0x14		// data register
#define UTSR1		0x20		// status register 1 offset
#define UTSR1_TNF	(1 << 2)	// tx FIFO not full (status bit)

	while ( ! (  * ( (volatile long *) (base + UTSR1)) & UTSR1_TNF ))
		; // busy wait while TX FIFO is full
	*(volatile unsigned char *) (base + UTDR)  = c;
	// *base2 = c;
#endif

/* ---------------------------------- PLEB2 (XSCALE PXA-255) --------------------------- */
#ifdef MACHINE_PLEB2 //XSCALE /* PXA 255 on PLEB2 */

/* Console port -- taken from kernel/include/platform/pleb2/console.h */
#define CONSOLE_OFFSET          0x100000

/* IO Base -- taken from kernel/include/arch/arm/xscale/cpu.h */
#define XSCALE_PXA255_IO_BASE   0x40000000

#define DATAR		0x00000000
#define STATUSR		0x00000014

/* TX empty bit -- uboot/include/asm/arch/hardware.h */
#define LSR_TEMT        (1 << 6)        /* Transmitter Empty */

volatile char *base  = (char *) (XSCALE_PXA255_IO_BASE + CONSOLE_OFFSET);

        /* wait for room in the tx FIFO on FFUART */
	while ( ! (  * ( (volatile long *) (base + STATUSR)) & LSR_TEMT ))
                ; // busy wait while TX FIFO is full
        *(volatile unsigned char *) (base + DATAR)  = c;

#endif	/* XSCALE */
#ifdef MACHINE_PB926
	{
		int res;
		do {
			res = pl011_tx_char(c);
		} while( res < 0);
	}
#endif /* MACHINE_PB926 */
	return(0);
}

/* --------------------------------- PB926 UART Driver -------------------------------- */
#ifdef MACHINE_PB926

extern struct pl011_uart uart;

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


#endif
