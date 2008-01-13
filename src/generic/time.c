/*
 * Time.
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */
#include <l4/types.h>
#include <l4/lib/mutex.h>
#include <l4/lib/printk.h>
#include <l4/generic/irq.h>
#include <l4/generic/scheduler.h>
#include <l4/generic/time.h>
#include INC_ARCH(exception.h)

/* TODO:
 * 1) Add RTC support.
 * 2) Need to calculate time since EPOCH,
 * 3) Jiffies must be initialised to a reasonable value.
 */

volatile u32 jiffies;

static inline void increase_jiffies(void)
{
	jiffies++;
}


static int noticks_noresched = 0;

/*
 * Check preemption anomalies:
 *
 * This checks how many times no rescheduling has occured even though ticks
 * reached zero. This suggests that preemption was enabled for more than a timer
 * interval. Normally, even if a preemption irq occured during a non-preemptive
 * state, preemption is *guaranteed* to occur before the next irq, provided that
 * the non-preemptive period is less than a timer irq interval (and it must be).
 *
 * Time:
 *
 * |-|---------------------|-|-------------------->
 * | V                     | V
 * | Preemption irq()      | Next irq.
 * V                       V
 * preempt_disabled()   preempt_enabled() && preemption;
 */
void check_noticks_noresched(void)
{
	if (!current->ticks_left)
		noticks_noresched++;

	if (noticks_noresched >= 2) {
		printk("Warning, no ticks and yet no rescheduling "
		       "for %d times.\n", noticks_noresched);
		printk("Spending more than a timer period"
		       " as nonpreemptive!!!\n");
	}
}

void update_process_times(void)
{
	struct ktcb *cur = current;

	BUG_ON(cur->ticks_left < 0);

	/*
	 * If preemption is disabled we stop reducing ticks when it reaches 0
	 * but set need_resched so that as soon as preempt-enabled, scheduling
	 * occurs.
	 */
	if (cur->ticks_left == 0) {
		need_resched = 1;
		// check_noticks_noresched();
		return;
	}
	// noticks_noresched = 0;

	if (in_kernel())
		cur->kernel_time++;
	else
		cur->user_time++;

	cur->ticks_left--;
	if (!cur->ticks_left)
		need_resched = 1;
}


int do_timer_irq(void)
{
	increase_jiffies();
	update_process_times();
	return IRQ_HANDLED;
}

