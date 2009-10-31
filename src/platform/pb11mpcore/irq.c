/*
 * Support for generic irq handling using platform irq controller (PL190)
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/platform.h>
#include <l4/generic/irq.h>
#include <l4/generic/time.h>
#include INC_PLAT(irq.h)
#include INC_PLAT(platform.h)
#include INC_ARCH(exception.h)
#include <l4/drivers/irq/pl190/pl190_vic.h>
#include <l4/drivers/timer/sp804/sp804_timer.h>

struct irq_chip irq_chip_array[IRQ_CHIPS_MAX];
#if 0
struct irq_chip irq_chip_array[IRQ_CHIPS_MAX] = {
	[0] = {
		.name = "Vectored irq controller",
		.level = 0,
		.cascade = IRQ_SIC,
		.offset = 0,
		.ops = {
			.init = pl190_vic_init,
			.read_irq = pl190_read_irq,
			.ack_and_mask = pl190_mask_irq,
			.unmask = pl190_unmask_irq,
		},
	},
	[1] = {
		.name = "Secondary irq controller",
		.level = 1,
		.cascade = IRQ_NIL,
		.offset = SIRQ_CHIP_OFFSET,
		.ops = {
			.init = pl190_sic_init,
			.read_irq = pl190_sic_read_irq,
			.ack_and_mask = pl190_sic_mask_irq,
			.unmask = pl190_sic_unmask_irq,
		},
	},
};
#endif

static int platform_timer_handler(void)
{
	sp804_irq_handler(PLATFORM_TIMER0_BASE);
	return do_timer_irq();
}

/* Built-in irq handlers initialised at compile time.
 * Else register with register_irq() */
struct irq_desc irq_desc_array[IRQS_MAX] = {
	[IRQ_TIMER01] = {
		.name = "Timer01",
		.chip = &irq_chip_array[0],
		.handler = platform_timer_handler,
	},
};

