/*
 * SP804 Primecell Timer offsets
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */
#ifndef __SP804_TIMER_H__
#define __SP804_TIMER_H__

#include INC_PLAT(platform.h)

#define SP804_TIMER01_BASE		PLATFORM_TIMER_BASE

#define SP804_TIMER1LOAD		(SP804_TIMER01_BASE + 0x0)
#define SP804_TIMER1VALUE		(SP804_TIMER01_BASE + 0x4)
#define SP804_TIMER1CONTROL		(SP804_TIMER01_BASE + 0x8)
#define SP804_TIMER1INTCLR		(SP804_TIMER01_BASE + 0xC)
#define SP804_TIMER1RIS			(SP804_TIMER01_BASE + 0x10)
#define SP804_TIMER1MIS			(SP804_TIMER01_BASE + 0x14)
#define SP804_TIMER1BGLOAD		(SP804_TIMER01_BASE + 0x18)
#define SP804_TIMER2OFFSET		0x20

void sp804_init(void);
void sp804_irq_handler(void);
void sp804_enable(int timer, int enable);
void sp804_set_irq(int timer, int enable);
#endif /* __SP804_TIMER_H__ */
