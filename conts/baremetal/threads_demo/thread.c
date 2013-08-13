/*
 * Test l4_mutex_control system call.
 *
 * Copyright (C) 2010 B Labs Ltd.
 *
 * Author: Bahadir Balban
 */

#include <string.h>
#include L4LIB_INC_ARCH(syslib.h)
#include L4LIB_INC_ARCH(syscalls.h)
#include <l4lib/lib/thread.h>

#define NTHREADS	6
#define dbg_printf printf

int thread_test_func1(void *arg)
{
	l4id_t tid = self_tid();

	printf("tid = %d is called.\n", tid);

	/* Wait for a while before exiting */
	int j = 0x400000;
	while (j--)
		;

	return tid;
}



int thread_test_func2(void *arg)
{
	l4id_t tid = self_tid();

	printf("tid = %d is called.\n", tid);

	/* Wait for a while before exiting */
	int j = 0x400000;
	while (j--)
		;

	thread_exit(0);

	return 0;
}


int thread_demo()
{
	struct l4_thread *thread[NTHREADS];
	int err;

	/* Create threads */
	for (int i = 0; i < NTHREADS; i++) {
		if (i % 2 ) {
			err = thread_create(thread_test_func1, 0,
					 TC_SHARE_SPACE, &thread[i]);

			if (err < 0) {
				dbg_printf("Thread create failed. "
					   "err=%d i= %d\n", err, i);
				return err;
			}
		} else {
			err = thread_create(thread_test_func2, 0,
					 TC_SHARE_SPACE, &thread[i]);
			if (err < 0) {
				dbg_printf("Thread create failed. "
					   "err=%d i= %d\n", err, i);
				return err;
			}
		}
	}

	/*
	 * Wait for all threads to exit successfully
	 */
	for (int i = 0; i < NTHREADS; i++) {
		if ((err = thread_wait(thread[i])) < 0) {
			dbg_printf("THREAD_WAIT failed. "
				   "err=%d\n", err);
			return err;
		}
	}

	dbg_printf("Thread test successful.\n");
	return 0;
}

int main(void)
{
	int err;

	__l4_threadlib_init();

	if ((err = thread_demo()) < 0)
		goto out_err;

	printf("THREAD DEMO:               -- PASSED --\n");
	return 0;

out_err:
	printf("THREAD DEMO:               -- FAILED --\n");
	return err;
}

