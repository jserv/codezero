/*
 * SP810 Primecell system controller offsets
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */

#ifndef __SP810_SYSCTRL_H__
#define __SP810_SYSCTRL_H__

#include INC_PLAT(platform.h)

/* FIXME: Fix the stupid uart driver and change to single definition of this! */
#if defined(read)
#undef read
#endif
#if defined(write)
#undef write
#endif

#define		read(a)		*((volatile unsigned int *)(a))
#define		write(v, a)	(*((volatile unsigned int *)(a)) = v)
#define		setbit(bit, a)	write(read(a) | bit, a)
#define		clrbit(bit, a)  write(read(a) & ~bit, a)
#define		devio(base, reg, bit, setclr)			\
			(setclr) ? setbit(bit, base + reg)	\
			: clrbit(bit, base + reg)

/* The SP810 system controller offsets */
#define SP810_BASE			PLATFORM_SYSCTRL_VIRTUAL
#define SP810_SCCTRL			(SP810_BASE + 0x0)
/* ... Fill in as needed. */


/* Set clock source for timers on this platform.
 * @timer: The index of timer you want to set the clock for.
 * On PB926 valid values are 0-4.
 *
 * @freq: The frequency you want to set the timer for.
 * On PB926 valid values are 32KHz = 0 (0 is 32Khz because that's
 * the default.) and 1MHz = non-zero.
 */
static inline void sp810_set_timclk(int timer, unsigned int freq)
{
	if (timer < 0 || timer > 3)
		return;

	freq ? setbit((1 << (15 + (2 * timer))), SP810_SCCTRL) :
	clrbit((1 << (15 + (2 * timer))), SP810_SCCTRL);

	return;
}

#endif /* __SP810_SYSCTRL_H__ */
