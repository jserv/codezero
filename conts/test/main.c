/*
 * Main function for all tests
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <l4/api/errno.h>
#include <container.h>
#include <capability.h>
#include <thread.h>
#include <tests.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4/api/space.h>

int exit_test_thread(void *arg)
{
	l4_exit(0);
	return 0;
}

int exit_test(void)
{
	int ret;
	struct task_ids ids;

	/* Create and run a new thread */
	if ((ret = thread_create(exit_test_thread, 0,
				 TC_SHARE_SPACE | TC_AS_PAGER,
				 &ids)) < 0) {
		printf("Top-level simple_pager creation failed.\n");
		goto out_err;
	}

	/* Wait on it */
	if ((ret = l4_thread_control(THREAD_WAIT, &ids)) >= 0)
		printf("Success. Paged child returned %d\n", ret);
	else
		printf("Error. Wait on (%d) failed. err = %d\n",
		       ids.tid, ret);

	return 0;

out_err:
	BUG();
}

int main(void)
{
	printf("%s: Container %s started\n",
	       __CONTAINER__, __CONTAINER_NAME__);

	//capability_test();

	//exit_test();

	/* Now quit to demo self-paging quit */
	l4_exit(0);
	/* Now quit by null pointer */
	//	*((int *)0) = 5;

	return 0;
}

