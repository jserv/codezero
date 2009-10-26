

int mutex_user_thread(void *arg)
{
	/* TODO: Create and access a mutex */
}

/*
 * This example demonstrates how a pager would
 * share part of its capabilities on the system
 * with its children.
 *
 * The example includes sharing of a mutex
 * capability with a paged-child.
 */
int multi_threaded_capability_sharing_example(void)
{
	struct capability *mutex_cap;
	int thread_retval;

	/*
	 * We are the first pager with capabilities to
	 * create new tasks, spaces, in its own container.
	 */
	pager_read_caps();

	/*
	 * We have all our capabilities private to us.
	 *
	 * If we create a new task, it won't be able to
	 * create and use userspace mutexes, because we
	 * hold mutex capabilities privately.
	 *
	 * Lets try it.
	 */

	/*
	 * Create new thread that will attempt
	 * a mutex operation, and die on us with a
	 * negative return code if it fails.
	 */
	if ((err = thread_create(mutex_user_thread, 0,
				 TC_SHARE_SPACE |
				 TC_AS_PAGER, &ids)) < 0) {
		printf("Mutex_user creation failed.\n");
		goto out_err;
	}

	/* Check on how the thread has done */
	if ((err = l4_thread_wait_on(ids, &thread_retval)) < 0) {
		print("Waiting on thread %d failed. err = %d\n",
		      ids->tid, err);
		goto out_err;
	}

	if (thread_retval == 0) {
		printf("Thread %d returned with success, where "
		       "we expected failure.\n", ids->tid);
		goto out_err;
	}

	/*
	 * Therefore, we share our capabilities with a
	 * collection so that our capabilities may be also
	 * used by them.
	 */

	/* Get our private mutex cap */
	mutex_cap = cap_get(CAP_TYPE_MUTEX);

	/* We have ability to create and use this many mutexes */
	printf("%s: We have ability to create/use %d mutexes\n",
	       self_tid(), mutex_cap->size);

	/* Split it */
	cap_new = cap_split(mutex_cap, 10, CAP_SPLIT_SIZE);

	/*
	 * Share the split part with paged-children.
	 *
	 * From this point onwards, any thread we create
	 * will have the ability to use mutexes, as defined
	 * by cap_new we created.
	 */
	l4_cap_share(cap_new, CAP_SHARE_PGGROUP, self_tid());

	/*
	 * Create new thread that will attempt
	 * a mutex operation, and die on us with a
	 * negative return code if it fails.
	 */
	if ((err = thread_create(mutex_user_thread, 0,
				 TC_SHARE_SPACE |
				 TC_AS_PAGER, &ids)) < 0) {
		printf("Mutex_user creation failed.\n");
		goto out_err;
	}

	/* Check on how the thread has done */
	if ((err = l4_thread_wait_on(ids, &thread_retval)) < 0) {
		printf("Waiting on thread %d failed. err = %d\n",
		      ids->tid, err);
		goto out_err;
	}

	if (thread_retval < 0) {
		printf("Thread %d returned with failure, where "
		       "we expected success.\n", ids->tid);
		goto out_err;
	}

out_err:
	BUG();
}








