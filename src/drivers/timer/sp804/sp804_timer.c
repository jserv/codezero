/*
 * SP804 Primecell Timer driver
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/drivers/timer/sp804/sp804_timer.h>

/* FIXME: Fix the shameful uart driver and change to single definition of this! */
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
			((setclr) ? setbit(bit, base + reg)	\
			: clrbit(bit, base + reg))

void sp804_irq_handler(void)
{
	/* Timer enabled as Periodic/Wrapper only needs irq clearing
	 * as it automatically reloads and wraps. */
	write(1, SP804_TIMER1INTCLR);
}

static inline void sp804_control(int timer, int bit, int setclr)
{
	unsigned long addr = SP804_TIMER1CONTROL + (timer ? SP804_TIMER2OFFSET : 0);
	setclr ? setbit(bit, addr) : clrbit(bit, addr);
}

/* Sets timer's run mode:
 * @periodic: periodic mode = 1, free-running = 0.
 */
#define SP804_PEREN			(1 << 6)
static inline void sp804_set_runmode(int timer, int periodic)
{
	sp804_control(timer, SP804_PEREN, periodic);
}

/* Sets timer's wrapping mode:
 * @oneshot: oneshot = 1, wrapping = 0.
 */
#define SP804_ONESHOT			(1 << 0)
static inline void sp804_set_wrapmode(int timer, int oneshot)
{
	sp804_control(timer, SP804_ONESHOT, oneshot);
}

/* Sets the operational width of timers.
 * In 16bit mode, top halfword is ignored.
 * @width: 32bit mode = 1; 16bit mode = 0
 */
#define SP804_32BIT			(1 << 1)
static inline void sp804_set_widthmode(int timer, int width)
{
	sp804_control(timer, SP804_32BIT, width);
}

/* Enable/disable timer:
 * @enable: enable = 1, disable = 0;
 */
#define SP804_ENABLE			(1 << 7)
void sp804_enable(int timer, int enable)
{
	sp804_control(timer, SP804_ENABLE, enable);
}

/* Enable/disable local irq register:
 * @enable: enable = 1, disable = 0
 */
#define SP804_IRQEN			(1 << 5)
void sp804_set_irq(int timer, int enable)
{
	sp804_control(timer, SP804_IRQEN, enable);
}

/* Loads timer with value in val */
static inline void sp804_load_value(int timer, u32 val)
{
	write(val, SP804_TIMER1LOAD + (timer ? SP804_TIMER2OFFSET : 0));
}

/* Returns current timer value */
static inline u32 sp804_read_value(int timer)
{
	return read(SP804_TIMER1VALUE + (timer ? SP804_TIMER2OFFSET : 0));
}

/* TODO: These are default settings! The values must be passed as arguments */
void sp804_init(void)
{
	/* 1 tick per usec */
	const int duration = 250;

	sp804_set_runmode(0, 1);   /* Periodic */
	sp804_set_wrapmode(0, 0);  /* Wrapping */
	sp804_set_widthmode(0, 1); /* 32 bit */
	sp804_set_irq(0, 1);	   /* Enable */
	sp804_load_value(0, duration);
}

