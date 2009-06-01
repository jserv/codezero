/*
 * Tests for userspace mutexes
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */
#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/mutex.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <tests.h>

/*
 * This structure is placed at the head of shared memory.
 * Tasks lock and write to rest of the page. Indicate that
 * they have run, and the later running task tests that both
 * processes has run and no data corruption occured on page_rest.
 */
struct shared_page {
	struct l4_mutex mutex;
	int child_has_run;
	int parent_has_run;
	char page_rest[];
};

static struct shared_page *shared_page;

/*
 * This test forks a parent task, creates a shared memory mapping, and makes two tasks
 * lock and write to almost the whole page 10 times with different values, and makes
 * the tasks test that the writes are all there without any wrong values. The amount of
 * time spent should justify a context switch and demonstrate that lock protects the
 * region.
 */
int user_mutex_test(void)
{
	pid_t child, parent;
	int map_size = PAGE_SIZE;
	int buf_size;
	void *base;

	/* Get parent pid */
	parent = getpid();


	/*
	 * Create a shared anonymous memory region. This can be
	 * accessed by both parent and child after a fork.
	 */
	if ((int)(base = mmap(0, map_size, PROT_READ | PROT_WRITE,
			      MAP_SHARED | MAP_ANONYMOUS, 0, 0)) < 0) {
		printf("%s: mmap for extended ipc buffer failed: %d\n",
			    __FUNCTION__, (int)base);
		goto out_err;
	} else
		printf("mmap: Anonymous shared buffer at %p\n", base);

	shared_page = base;

	/* Find the buffer size */
	buf_size = map_size - sizeof(*shared_page);
	printf("Total buffer size: %d\n", buf_size);

	/* Initialize the mutex */
	l4_mutex_init(&shared_page->mutex);

	/* Initialize buffer with unique child value */
	printf("Initializing shared page with child values.\n");
	shared_page->child_has_run = 0;
	shared_page->parent_has_run = 0;
	for (int i = 0; i < buf_size; i++)
		shared_page->page_rest[i] = 0;

	/* Fork the current task */
	if ((child = fork()) < 0) {
		printf("%s: Fork failed with %d\n", __FUNCTION__, errno);
		goto out_err;
	}

	if (child)
		printf("%d: Created child with pid %d\n", getpid(), child);
	else
		printf("Child %d running.\n", getpid());

	/* Child locks and produces */
	if (child == 0) {

		for (int x = 0; x < 255; x++) {
			/* Lock */
			printf("Child locking page.\n");
			l4_mutex_lock(&shared_page->mutex);

			printf("Child locked page.\n");

			l4_thread_switch(0);
			/* Get sample value */
			//char val = shared_page->page_rest[0];

			/* Write a unique child value to whole buffer */
			for (int i = 0; i < buf_size; i++) {
#if 0
				/* Check sample is same in all */
				if (shared_page->page_rest[i] != val) {
					printf("Sample values dont match. "
					       "page_rest[%d] = %c, "
					       "val = %c\n", i,
					       shared_page->page_rest[i], val);
					goto out_err;
				}
#endif
				shared_page->page_rest[i]--;
			}
			printf("Child produced. Unlocking...\n");
			/* Unlock */
			l4_mutex_unlock(&shared_page->mutex);
			printf("Child unlocked page.\n");
		}
		/* Sync with the parent */
		l4_send(parent, L4_IPC_TAG_SYNC);

	/* Parent locks and consumes */
	} else {
		for (int x = 0; x < 255; x++) {
			printf("Parent locking page.\n");

			/* Lock the page */
			l4_mutex_lock(&shared_page->mutex);
			printf("Parent locked page.\n");
			l4_thread_switch(0);

	//		printf("Parent reading:\n");

			/* Test and consume the page */
			for (int i = 0; i < buf_size; i++) {
	//			printf("%c", shared_page->page_rest[i]);
				/* Test that child has produced */
#if 0
				if (shared_page->page_rest[i] != 'c') {
					printf("Child not produced. "
					       "page_rest[%d] = %c, "
					       "expected = 'c'\n",
					       i, shared_page->page_rest[i]);
					BUG();
					goto out_err;
				}
#endif
				/* Consume the page */
				shared_page->page_rest[i]++;
			}
	//		printf("\n\n");
			printf("Parent consumed. Unlocking...\n");
			l4_mutex_unlock(&shared_page->mutex);
			printf("Parent unlocked page.\n");
		}
		/* Sync with the child */
		l4_receive(child);

		printf("Parent checking validity of values.\n");
		for (int i = 0; i < 255; i++)
			if (shared_page->page_rest[i] != 0)
				goto out_err;

		printf("USER MUTEX TEST:    -- PASSED --\n");
	}
	return 0;

out_err:
	printf("USER MUTEX TEST:    -- FAILED --\n");
	return -1;
}





