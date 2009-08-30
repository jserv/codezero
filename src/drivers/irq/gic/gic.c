/*
 * PL190 Vectored irq controller support.
 *
 * This is more pb926 specific as it also touches the SIC, a partial irq
 * controller.Normally, irq controller must be independent and singular. Later
 * other generic code should make thlongwork in cascaded setup.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#include <l4/lib/bit.h>
#include <l4/drivers/irq/pl190/pl190_vic.h>

/* FIXME: Fix the stupid uart driver and change to single definition of this! */
#if defined(read)
#undef read
#endif
#if defined(write)
#undef write
#endif

#define	read(a)				*((volatile unsigned int *)(a))
#define	write(v, a)			(*((volatile unsigned int *)(a)) = v)
#define	setbit(bitvect, a)		write(read(a) | (bitvect), a)
#define	clrbit(bitvect, a)		write(read(a) & ~(bitvect), a)
#define	devio(base, reg, bitvect, setclr)			\
		((setclr) ? setbit(bitvect, (base + reg))	\
		: clrbit(bitvect, (base + reg)))

/* Returns the irq number on this chip converting the irq bitvector */
int pl190_read_irq(void)
{
	/* This also correctly returns a negative value for a spurious irq. */
	return 31 - __clz(read(PL190_VIC_IRQSTATUS));
}

void pl190_mask_irq(int irq)
{
	/* Reading WO registers blows QEMU/PB926.
	 * setbit((1 << irq), PL190_VIC_INTENCLEAR); */
	write(1 << irq, PL190_VIC_INTENCLEAR);
}

/* Ack is same as mask */
void pl190_ack_irq(int irq)
{
	pl190_mask_irq(irq);
}

void pl190_unmask_irq(int irq)
{
	setbit(1 << irq, PL190_VIC_INTENABLE);
}

int pl190_sic_read_irq(void)
{
	return 32 - __clz(read(PL190_SIC_STATUS));
}

void pl190_sic_mask_irq(int irq)
{
	write(1 << irq, PL190_SIC_ENCLR);
}

void pl190_sic_ack_irq(int irq)
{
	pl190_sic_mask_irq(irq);
}

void pl190_sic_unmask_irq(int irq)
{
	setbit(1 << irq, PL190_SIC_ENSET);
}

/* Initialises the primary and secondary interrupt controllers */
void pl190_vic_init(void)
{
	/* Clear all interrupts */
	write(0, PL190_VIC_INTENABLE);
	write(0xFFFFFFFF, PL190_VIC_INTENCLEAR);

	/* Set all irqs as normal IRQs (i.e. not FIQ) */
	write(0, PL190_VIC_INTSELECT);
	/* TODO: Is there a SIC_IRQ_SELECT for irq/fiq ??? */

	/* Disable user-mode access to VIC registers */
	write(1, PL190_VIC_PROTECTION);

	/* Clear software interrupts */
	write(0xFFFFFFFF, PL190_VIC_SOFTINTCLEAR);

	/* At this point, all interrupts are cleared and disabled.
	 * the controllers are ready to receive interrupts, if enabled. */
	return;
}

void pl190_sic_init(void)
{
	write(0, PL190_SIC_ENABLE);
	write(0xFFFFFFFF, PL190_SIC_ENCLR);
	/* Disable SIC-to-PIC direct routing of individual irq lines on SIC */
	write(0xFFFFFFFF, PL190_SIC_PICENCLR);
}

