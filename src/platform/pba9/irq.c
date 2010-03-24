/*
 * Platform generic irq handling
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/platform.h>
#include <l4/generic/irq.h>
#include <l4/generic/time.h>
#include INC_PLAT(irq.h)
#include INC_PLAT(platform.h)
#include INC_ARCH(exception.h)
#include <l4/drivers/irq/gic/gic.h>

extern struct gic_data gic_data[IRQ_CHIPS_MAX];

struct irq_chip irq_chip_array[IRQ_CHIPS_MAX] = {
	[0] = {
		.name = "VX-A9 GIC",
		.level = 0,
		.cascade = IRQ_NIL,
		.start = 0,
		.end = IRQ_OFFSET + IRQS_MAX,
		.data = &gic_data[0],
		.ops = {
			.init = gic_dummy_init,
			.read_irq = gic_read_irq,
			.ack_and_mask = gic_ack_and_mask,
			.unmask = gic_unmask_irq,
		},
	},
};

