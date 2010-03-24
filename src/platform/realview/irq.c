/*
 * Support for generic irq handling.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/irq.h>
#include <l4/generic/time.h>
#include INC_PLAT(offsets.h)
#include INC_PLAT(irq.h)
#include <l4/platform/realview/timer.h>

static int platform_timer_handler(struct irq_desc *desc)
{
	timer_irq_clear(PLATFORM_TIMER0_VBASE);

	return do_timer_irq();
}

/*
 * Built-in irq handlers initialised at compile time.
 * Else register with register_irq()
 */
struct irq_desc irq_desc_array[IRQS_MAX] = {
	[IRQ_TIMER0] = {
		.name = "Timer0",
		.chip = &irq_chip_array[0],
		.handler = platform_timer_handler,
	},
};

