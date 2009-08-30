/*
 * Ties up platform timer with generic timer api
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */
#include <l4/generic/irq.h>
#include <l4/generic/platform.h>
#include INC_PLAT(platform.h)
#include <l4/drivers/timer/sp804/sp804_timer.h>
#include <l4/drivers/misc/sp810/sp810_sysctrl.h>

void timer_init(void)
{
	/* Set timer 0 to 1MHz */
	sp810_set_timclk(0, 1);

	/* Initialise timer */
	sp804_init();
}

void timer_start(void)
{
	irq_enable(IRQ_TIMER01);
	sp804_set_irq(0, 1);	/* Enable timer0 irq */
	sp804_enable(0, 1);	/* Enable timer0 */
}

