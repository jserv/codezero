/*
 * Copyright (C) 2010 B Labs Ltd.
 *
 * l4_thread_control performance tests
 *
 * Author: Bahadir Balban
 */

#include <l4lib/macros.h>
#include L4LIB_INC_ARCH(syslib.h)
#include L4LIB_INC_ARCH(syscalls.h)
#include <l4lib/lib/thread.h>
#include <l4lib/perfmon.h>
#include <perf.h>
#include <tests.h>
#include <string.h>

struct perfmon_cycles thread_switch_cycles;
struct perfmon_cycles space_switch_cycles;

static int indicate_switch = 0;

void thread_switcher_thread(void *arg)
{
	l4id_t parent = ((struct task_ids *)arg)->tid;

	printf("%s: Running\n", __FUNCTION__);
	/* Wait until parent signals us to switch */
	while (!indicate_switch)
		l4_thread_switch(parent);

	/*
	 * Now do one last switch, which will
	 * be used in the actual switch measurement
	 */
	l4_thread_switch(parent);
}

void perf_measure_thread_switch_simple(void)
{
	struct task_ids thread_switcher;
	struct task_ids selfid;
	l4_getid(&selfid);

	/*
	 * Initialize structures
	 */
	memset(&thread_switch_cycles, 0, sizeof (struct perfmon_cycles));
	thread_switch_cycles.min = ~0; /* Init as maximum possible */

	/* Start the counter */
	perfmon_reset_start_cyccnt();

	l4_thread_switch(0);

	perfmon_record_cycles(&thread_switch_cycles, "THREAD_SWITCH");

	/*
	 * Calculate average
	 */
	thread_switch_cycles.avg =
		thread_switch_cycles.total / thread_switch_cycles.ops;

	/*
	 * Print results
	 */
	printf("%s took %llu cycles, %llu min, %llu max, %llu avg, in %llu ops.\n",
	       "THREAD_SWITCH",
	       thread_switch_cycles.min,
	       thread_switch_cycles.min * USEC_MULTIPLIER,
	       thread_switch_cycles.max * USEC_MULTIPLIER,
	       thread_switch_cycles.avg * USEC_MULTIPLIER,
	       thread_switch_cycles.ops);
}

void perf_measure_thread_switch(void)
{
	struct task_ids thread_switcher;
	struct task_ids selfid;
	l4_getid(&selfid);

	/*
	 * Initialize structures
	 */
	memset(&thread_switch_cycles, 0, sizeof (struct perfmon_cycles));
	thread_switch_cycles.min = ~0; /* Init as maximum possible */

	/* Create switcher thread */
	l4_thread_control(THREAD_CREATE | TC_SHARE_SPACE, &selfid);

	/* Copy ids of created task */
	memcpy(&thread_switcher, &selfid, sizeof(struct task_ids));

	/* Switch to the thread to ensure it runs at least once */
	l4_thread_switch(thread_switcher.tid);

	/* Start the counter */
	perfmon_reset_start_cyccnt();

	/* Set the switch indicator */
	indicate_switch = 1;

	/*
	 * Switch to thread.
	 *
	 * We should be in full control here, because
	 * the thread must have run out of its time slices.
	 */
	l4_thread_switch(thread_switcher.tid);

	/*
	 * By this time, the switcher thread must have done a
	 * thread switch back to us
	 */
	perfmon_record_cycles(&thread_switch_cycles, "THREAD_SWITCH");

	/*
	 * Calculate average
	 */
	thread_switch_cycles.avg =
		thread_switch_cycles.total / thread_switch_cycles.ops;

	/*
	 * Print results
	 */
	printf("%s took %llu cycles, %llu min, %llu max, %llu avg, in %llu ops.\n",
	       "THREAD_SWITCH",
	       thread_switch_cycles.min,
	       thread_switch_cycles.min * USEC_MULTIPLIER,
	       thread_switch_cycles.max * USEC_MULTIPLIER,
	       thread_switch_cycles.avg * USEC_MULTIPLIER,
	       thread_switch_cycles.ops);


	/* Destroy the thread */
	l4_thread_control(THREAD_DESTROY, &thread_switcher);
}


void perf_measure_space_switch(void)
{

}


void perf_measure_tswitch(void)
{
	perf_measure_thread_switch_simple();
	perf_measure_space_switch();
}
