#ifndef __TIMER_H__
#define	__TIMER_H__

/*
 * Timer specific things are here
 */
#include <l4lib/mutex.h>
#include <l4/lib/list.h>

/*
  * Structure representing the sleeping tasks, 
  * tgid: tgid of sleeping task
  * wait_count: time left, in microseconds, after which task
  * 			will be signalled to get out of sleep
  */
struct timer_task {
	struct link list;
	l4id_t tgid;
	unsigned int wait_count;
};

/*
  * Timer structure,
  * base: base address of sp804 timer encapsulated
  * count: Count in microseconds from the start of this timer
  * tasklist: list of tasks sleeping for some value of count
  * lock: lock protecting the corruption of tasklist
  */
struct sp804_timer {
	unsigned int base;
	unsigned int count;
	struct link tasklist;
	struct l4_mutex lock;
};

#endif /* __TIMER_H__ */
