/*
 * PLXXX Generic Interrupt Controller support.
 *
 * This is more ARM Realview EB/PB
 * Copyright (C) 2009-2010 B Labs Ltd.
 * Author: Prem Mallappa <prem.mallappa@b-labs.co.uk>
 */

#include <l4/lib/bit.h>
#include <l4/lib/printk.h>
#include <l4/generic/irq.h>
#include INC_PLAT(irq.h)
#include INC_SUBARCH(mmu_ops.h)  /* for dmb/dsb() */
#include <l4/drivers/irq/gic/gic.h>

volatile struct gic_data gic_data[IRQ_CHIPS_MAX];

static inline struct gic_data *get_gic_data(l4id_t irq)
{
	struct irq_chip *chip = irq_desc_array[irq].chip;
	if (chip)
		return (struct gic_data *)irq_desc_array[irq].chip->data;
	else
		return 0;
}

/* Returns the irq number on this chip converting the irq bitvector */
l4id_t gic_read_irq(void *data)
{
	int irq;
	volatile struct gic_data *gic = (struct gic_data *)data;
	irq = gic->cpu->ack & 0x1ff;

	if (irq == 1023)
		return -1023;			/* Spurious */

	return irq;
}

void gic_mask_irq(l4id_t irq)
{
	u32 offset = irq >> 5; /* offset = irq / 32, avoiding division */
	volatile struct gic_data *gic = get_gic_data(irq);
	gic->dist->clr_en[offset] = 1 << (irq % 32);
}

void gic_unmask_irq(l4id_t irq)
{
	volatile struct gic_data *gic = get_gic_data(irq);

	u32 offset = irq >> 5 ; /* offset = irq / 32 */
	gic->dist->set_en[offset] = 1 << (irq % 32);
}

void gic_ack_irq(l4id_t irq)
{
	u32 offset = irq >> 5; /* offset = irq / 32, avoiding division */
	volatile struct gic_data *gic = get_gic_data(irq);
	gic->dist->clr_en[offset] = 1 << (irq % 32);
	gic->cpu->eoi = irq;
}

void gic_ack_and_mask(l4id_t irq)
{
	gic_ack_irq(irq);
	gic_mask_irq(irq);
}

void gic_set_pending(l4id_t irq)
{
	u32 offset = irq >> 5; /* offset = irq / 32, avoiding division */
	volatile struct gic_data *gic = get_gic_data(irq);
	gic->dist->set_pending[offset] = 1 << (irq % 32);
}

void gic_clear_pending(l4id_t irq)
{
	u32 offset = irq >> 5; /* offset = irq / 32, avoiding division */
	volatile struct gic_data *gic = get_gic_data(irq);
	gic->dist->clr_pending[offset] = 1 << (irq % 32);
}


void gic_cpu_init(int idx, unsigned long base)
{
	struct gic_cpu *cpu;
	cpu = gic_data[idx].cpu = (struct gic_cpu *)base;

	/* Disable */
	cpu->control = 0;

	cpu->prio_mask = 0xf0;
	cpu->bin_point = 3;

	cpu->control = 1;
}

void gic_dist_init(int idx, unsigned long base)
{
	int i, irqs_per_word; 	/* Interrupts per word */
	struct gic_dist *dist;
	dist = gic_data[idx].dist = (struct gic_dist *)(base);

	/* Surely disable GIC */
	dist->control = 0;

	/* 32*(N+1) interrupts supported */
	int nirqs = 32 * ((dist->type & 0x1f) + 1);
	if (nirqs > IRQS_MAX)
		nirqs = IRQS_MAX;

	/* Clear all interrupts */
	irqs_per_word = 32;
	for(i = 0; i < nirqs ; i+=irqs_per_word) {
		dist->clr_en[i/irqs_per_word] = 0xffffffff;
	}

	/* Clear all pending interrupts */
	for(i = 0; i < nirqs ; i+=irqs_per_word) {
		dist->clr_pending[i/irqs_per_word] = 0xffffffff;
	}

	/* Set all irqs as normal priority, 8 bits per interrupt */
	irqs_per_word = 4;
	for(i = 32; i < nirqs ; i+=irqs_per_word) {
		dist->priority[i/irqs_per_word] = 0xa0a0a0a0;
	}

	/* Set all target to cpu0, 8 bits per interrupt */
	for(i = 32; i < nirqs ; i+=irqs_per_word) {
		dist->target[i/irqs_per_word] = 0x01010101;
	}

	/* Configure all to be level-sensitive, 2 bits per interrupt */
	irqs_per_word = 16;
	for(i = 32; i < nirqs ; i+=irqs_per_word) {
		dist->config[i/irqs_per_word] = 0x00000000;
	}

	/* Enable GIC Distributor */
	dist->control = 1;
}


/* Some functions, may be helpful */
void gic_set_target(u32 irq, u32 cpu)
{
	/* cpu is a mask, not cpu number */
	cpu &= 0xF;
	irq &= 0xFF;
	volatile struct gic_data *gic = get_gic_data(irq);
	u32 offset = irq >> 2; /* offset = irq / 4 */
	gic->dist->target[offset] |= (cpu << ((irq % 4) * 8));
}

u32 gic_get_target(u32 irq)
{
	/* cpu is a mask, not cpu number */
	unsigned int target;
	irq &= 0xFF;
	u32 offset = irq >> 2; /* offset = irq / 4 */
	volatile struct gic_data *gic = get_gic_data(irq);
	target = gic->dist->target[offset];
	target >>= ((irq % 4) * 8);

	return target & 0xFF;
}

void gic_set_priority(u32 irq, u32 prio)
{
	/* cpu is a mask, not cpu number */
	prio &= 0xF;
	irq &= 0xFF;
	u32 offset = irq >> 3; /* offset = irq / 8 */
	volatile struct gic_data *gic = get_gic_data(irq);
	/* target = cpu << ((irq % 4) * 4) */
	gic->dist->target[offset] |= (prio << (irq & 0x1C));
}

u32 gic_get_priority(u32 irq)
{
	/* cpu is a mask, not cpu number */
	irq &= 0xFF;
	u32 offset = irq >> 3; /* offset = irq / 8 */
	volatile struct gic_data *gic = get_gic_data(irq);
	return gic->dist->target[offset] & (irq & 0xFC);
}

#define TO_MANY		0	/* to all specified in a CPU mask */
#define TO_OTHERS	1	/* all but me */
#define TO_SELF		2	/* just to the requesting CPU */

#define CPU_MASK_BIT  16
#define TYPE_MASK_BIT 24

void gic_send_ipi(int cpu, int ipi_cmd)
{
	/* if cpu is 0, then ipi is sent to self
	 * if cpu has exactly 1 bit set, the ipi to just that core
	 * if cpu has a mask, sent to all but current
	 */
	struct gic_dist *dist = gic_data[0].dist;

	ipi_cmd &= 0xf;
	cpu &= 0xff;

	dsb();

	if (cpu == 0)				/* Self */
		dist->soft_int = (TO_SELF << 24) | ipi_cmd;
	else if ((cpu & (cpu-1)) == 0)		/* Exactly to one CPU */
		dist->soft_int = (TO_MANY << 24) | (cpu << 16) | ipi_cmd;
	else				/* All but me */
		dist->soft_int = (TO_OTHERS << 24) | (cpu << 16) | ipi_cmd;

}

/* Make the generic code happy :) */
void gic_dummy_init()
{

}
