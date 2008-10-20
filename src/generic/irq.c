/*
 * Kernel irq handling (core irqs like timer). Also hope to add thread-level
 * irq handling in the future.
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */
#include <l4/config.h>
#include <l4/macros.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/platform.h>
#include <l4/generic/tcb.h>
#include <l4/generic/irq.h>
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include INC_PLAT(irq.h)
#include INC_ARCH(exception.h)

/* This enables the lower chip on the current chip, if such chaining exists. */
static inline void cascade_irq_chip(struct irq_chip *this_chip)
{
	if (this_chip->cascade >= 0) {
		BUG_ON(IRQ_CHIPS_MAX == 1);
		this_chip->ops.unmask(this_chip->cascade);
	}
}

void irq_controllers_init(void)
{
	struct irq_chip *this_chip;

	for (int i = 0; i < IRQ_CHIPS_MAX; i++) {
		this_chip = irq_chip_array + i;
		/* Initialise the irq chips (e.g. reset all registers) */
		this_chip->ops.init();
		/* Enable cascaded irqs if needed */
		cascade_irq_chip(this_chip);
	}
}

int global_irq_index(void)
{
	struct irq_chip *this_chip;
	int irq_index = 0;

	/* Loop over irq chips from top to bottom until
	 * the actual irq on the lowest chip is found */
	for (int i = 0; i < IRQ_CHIPS_MAX; i++) {
		this_chip = irq_chip_array + i;
		BUG_ON((irq_index = this_chip->ops.read_irq()) < 0);
		if (irq_index != this_chip->cascade) {
			irq_index += this_chip->offset;
			/* Found the real irq, return */
			break;
		}
		/* Hit the cascading irq. Continue on next irq chip. */
	}
	return irq_index;
}

void do_irq(void)
{
	int irq_index = global_irq_index();
	struct irq_desc *this_irq = irq_desc_array + irq_index;

	/* TODO: This can be easily done few instructions quicker by some
	 * immediate read/disable/enable_all(). We stick with this clear
	 * implementation for now. */
	irq_disable(irq_index);
	enable_irqs();
	/* TODO: Call irq_thread_notify(irq_index) for threaded irqs. */
	BUG_ON(!this_irq->handler);
	if (this_irq->handler() != IRQ_HANDLED) {
		printk("Spurious or broken irq\n"); BUG();
	}
	irq_enable(irq_index);

	/* Process any pending flags for currently runnable task */
	task_process_pending_flags();
}




