/*
 * SP804 Primecell Timer offsets
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */
#ifndef __SP804_TIMER_H__
#define __SP804_TIMER_H__

#include INC_PLAT(platform.h)

/* Run mode of timers */
#define SP804_TIMER_RUNMODE_FREERUN	0
#define SP804_TIMER_RUNMODE_PERIODIC	1

/* Wrap mode of timers */
#define SP804_TIMER_WRAPMODE_WRAPPING	0
#define SP804_TIMER_WRAPMODE_ONESHOT	1

/* Operational width of timer */
#define SP804_TIMER_WIDTH16BIT	0
#define SP804_TIMER_WIDTH32BIT	1

/* Enable/disable irq on timer */
#define SP804_TIMER_IRQDISABLE	0
#define SP804_TIMER_IRQENABLE	1

/* Register offsets */
#define SP804_TIMERLOAD		0x0
#define SP804_TIMERVALUE	0x4
#define SP804_TIMERCONTROL	0x8
#define SP804_TIMERINTCLR	0xC
#define SP804_TIMERRIS		0x10
#define SP804_TIMERMIS		0x14
#define SP804_TIMERBGLOAD	0x18

void sp804_init(unsigned int timer_base, int runmode, int wrapmode, \
		int width, int irq_enable);
void sp804_irq_handler(unsigned int timer_base);
void sp804_enable(unsigned int timer_base, int enable);
void sp804_set_irq(unsigned int timer_base, int enable);

#endif /* __SP804_TIMER_H__ */
