/*
 * SP804 Primecell Timer offsets
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */
#ifndef __SP804_TIMER_H__
#define __SP804_TIMER_H__

#define SP804_TIMER2_OFFSET	0x20

/* Base address of Timers for differen platforms */
#if defined(PLATFORM_PB926)
#define	TIMER0_PHYS_BASE		0x101E2000
#define	TIMER1_PHYS_BASE		(TIMER0_PHYS_BASE + SP804_TIMER2_OFFSET)
#define	TIMER2_PHYS_BASE		0x101E3000
#define	TIMER3_PHYS_BASE		(TIMER2_PHYS_BASE + SP804_TIMER2_OFFSET)
#elif defined(PLATFORM_EB)
#define	TIMER0_PHYS_BASE		0x10011000
#define	TIMER1_PHYS_BASE		(TIMER0_PHYS_BASE + SP804_TIMER2_OFFSET)
#define	TIMER2_PHYS_BASE		0x10012000
#define	TIMER3_PHYS_BASE		(TIMER2_PHYS_BASE + SP804_TIMER2_OFFSET)
#elif defined(PLATFORM_PB11MPCORE)
#define	TIMER0_PHYS_BASE		0x10011000
#define	TIMER1_PHYS_BASE		(TIMER0_PHYS_BASE + SP804_TIMER2_OFFSET)
#define	TIMER2_PHYS_BASE		0x10012000
#define	TIMER3_PHYS_BASE		(TIMER2_PHYS_BASE + SP804_TIMER2_OFFSET)
#define	TIMER4_PHYS_BASE		0x10018000
#define	TIMER5_PHYS_BASE		(TIMER4_PHYS_BASE + SP804_TIMER2_OFFSET)
#define	TIMER6_PHYS_BASE		0x10019000
#define	TIMER7_PHYS_BASE		(TIMER6_PHYS_BASE + SP804_TIMER2_OFFSET)
#endif

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

struct sp804_timer {
	unsigned int base;
};

void sp804_init(unsigned int timer_base, int runmode, int wrapmode, \
		int width, int irq_enable);
void sp804_irq_handler(unsigned int timer_base);
void sp804_enable(unsigned int timer_base, int enable);
void sp804_set_irq(unsigned int timer_base, int enable);

unsigned int sp804_read_value(unsigned int timer_base);

#endif /* __SP804_TIMER_H__ */
