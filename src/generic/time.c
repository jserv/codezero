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
#include <l4/generic/space.h>
#include INC_ARCH(exception.h)
#include <l4/api/syscall.h>
#include <l4/api/errno.h>

/* TODO:
 * 1) Add RTC support.
 * 2) Need to calculate time since EPOCH,
 * 3) Jiffies must be initialised to a reasonable value.
 */

volatile u32 jiffies = 0;

static inline void increase_jiffies(void)
{
	jiffies++;
}


/* Internal representation of time since epoch */
struct time_info {
	int reader;
	u32 thz;	/* Ticks in this hertz so far */
	u64 sec;	/* Seconds so far */
};

/* Used by posix systems */
struct timeval {
	int tv_sec;
	int tv_usec;
};

static struct time_info systime = { 0 };

/*
 * A very basic (probably erroneous)
 * rule-of-thumb time calculation.
 */
void update_system_time(void)
{
	/* Did we interrupt a reader? Tell it to retry */
	if (systime.reader)
		systime.reader = 0;

	/* Increase just like jiffies, but reset every HZ */
	systime.thz++;

	/* On every HZ increase seconds */
	if (systime.thz == HZ) {
		systime.thz = 0;
		systime.sec++;
	}
}

/* Read system time */
int sys_time(struct syscall_args *args)
{
       	struct timeval *tv = (struct timeval *)args->r0;
	int set = (int)args->r1;
	int retries = 20;

	if (check_access((unsigned long)tv, sizeof(*tv), MAP_USR_RW_FLAGS) < 0)
		return -EINVAL;

	/* Get time */
	if (!set) {
		while(retries > 0) {
			systime.reader = 1;
			tv->tv_sec = systime.sec;
			tv->tv_usec = 1000000 * systime.thz / HZ;

			retries--;
			if (systime.reader)
				break;
		}

		/*
		 * No need to reset reader since it will be reset
		 * on next timer. If no retries return busy.
		 */
		if (!retries)
			return -EBUSY;
		else
			return 0;

	/* Set */
	} else {
		/*
		 * Setting the time not supported yet.
		 */
		return -ENOSYS;
	}
}

void update_process_times(void)
{
	struct ktcb *cur = current;

	BUG_ON(cur->ticks_left < 0);

	if (cur->ticks_left == 0) {
		need_resched = 1;
		return;
	}

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
	update_system_time();

	return IRQ_HANDLED;
}

