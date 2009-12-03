/*
 * SP804 Primecell Timer driver
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <sp804_timer.h>

#define		read(a)		*((volatile unsigned int *)(a))
#define		write(v, a)	(*((volatile unsigned int *)(a)) = v)
#define		setbit(bit, a)	write(read(a) | bit, a)
#define		clrbit(bit, a)  write(read(a) & ~bit, a)

extern void timer_irq_handler(void);

void sp804_irq_handler(unsigned int timer_base)
{
	/*
	 * Timer enabled as Periodic/Wrapper only needs irq clearing
	 * as it automatically reloads and wraps
	 */
	write(1, (timer_base + SP804_TIMERINTCLR));
	timer_irq_handler();
}

static inline void sp804_control(unsigned int timer_base, int bit, int setclr)
{
	unsigned long addr = (timer_base + SP804_TIMERCONTROL);
	setclr ? setbit(bit, addr) : clrbit(bit, addr);
}

/*
 * Sets timer's run mode:
 * @periodic: periodic mode = 1, free-running = 0.
 */
#define SP804_PEREN			(1 << 6)
static inline void sp804_set_runmode(unsigned int timer_base, int periodic)
{
	sp804_control(timer_base, SP804_PEREN, periodic);
}

/*
 * Sets timer's wrapping mode:
 * @oneshot: oneshot = 1, wrapping = 0.
 */
#define SP804_ONESHOT			(1 << 0)
static inline void sp804_set_wrapmode(unsigned int timer_base, int oneshot)
{
	sp804_control(timer_base, SP804_ONESHOT, oneshot);
}

/*
 * Sets the operational width of timers.
 * In 16bit mode, top halfword is ignored.
 * @width: 32bit mode = 1; 16bit mode = 0
 */
#define SP804_32BIT			(1 << 1)
static inline void sp804_set_widthmode(unsigned int timer_base, int width)
{
	sp804_control(timer_base, SP804_32BIT, width);
}

/*
 * Enable/disable timer:
 * @enable: enable = 1, disable = 0;
 */
#define SP804_ENABLE			(1 << 7)
void sp804_enable(unsigned int timer_base, int enable)
{
	sp804_control(timer_base, SP804_ENABLE, enable);
}

/*
 * Enable/disable local irq register:
 * @enable: enable = 1, disable = 0
 */
#define SP804_IRQEN			(1 << 5)
void sp804_set_irq(unsigned int timer_base, int enable)
{
	sp804_control(timer_base, SP804_IRQEN, enable);
}

/* Loads timer with value in val */
static inline void sp804_load_value(unsigned int timer_base, unsigned int val)
{
	write(val, (timer_base + SP804_TIMERLOAD));
}

/* Returns current timer value */
unsigned int sp804_read_value(unsigned int timer_base)
{
	return read(timer_base + SP804_TIMERVALUE);
}

/* TODO: Define macro values for duration */
void sp804_init(unsigned int timer_base, int run_mode, int wrap_mode, \
				  int width, int irq_enable)
{
	/* 1 tick per usec */
	const int duration = 250;

	sp804_set_runmode(timer_base, run_mode);   /* Periodic */
	sp804_set_wrapmode(timer_base, wrap_mode);  /* Wrapping */
	sp804_set_widthmode(timer_base, width); /* 32 bit */
	sp804_set_irq(timer_base, irq_enable);	   /* Enable */
	sp804_load_value(timer_base, duration);
}

