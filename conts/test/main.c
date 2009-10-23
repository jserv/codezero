/*
 * Main function for this container
 */
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/exregs.h>
#include <l4lib/init.h>
#include <l4/api/capability.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/api/thread.h>
#include <l4/api/space.h>
#include <l4/api/errno.h>
#include <container.h>

static struct capability cap_array[30];
static int total_caps;

void print_capability(struct capability *cap)
{
	printf("Capability id:\t\t\t%d\n", cap->capid);
	printf("Capability resource id:\t\t%d\n", cap->resid);
	printf("Capability owner id:\t\t%d\n",cap->owner);

	switch (cap_type(cap)) {
	case CAP_TYPE_TCTRL:
		printf("Capability type:\t\t%s\n", "Thread Control");
		break;
	case CAP_TYPE_EXREGS:
		printf("Capability type:\t\t%s\n", "Exchange Registers");
		break;
	case CAP_TYPE_MAP:
		printf("Capability type:\t\t%s\n", "Map");
		break;
	case CAP_TYPE_IPC:
		printf("Capability type:\t\t%s\n", "Ipc");
		break;
	case CAP_TYPE_UMUTEX:
		printf("Capability type:\t\t%s\n", "Mutex");
		break;
	case CAP_TYPE_QUANTITY:
		printf("Capability type:\t\t%s\n", "Quantitative");
		break;
	default:
		printf("Capability type:\t\t%s\n", "Unknown");
		break;
	}

	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
		printf("Capability resource type:\t%s\n", "Thread");
		break;
	case CAP_RTYPE_TGROUP:
		printf("Capability resource type:\t%s\n", "Thread Group");
		break;
	case CAP_RTYPE_SPACE:
		printf("Capability resource type:\t%s\n", "Space");
		break;
	case CAP_RTYPE_CONTAINER:
		printf("Capability resource type:\t%s\n", "Container");
		break;
	case CAP_RTYPE_UMUTEX:
		printf("Capability resource type:\t%s\n", "Mutex");
		break;
	case CAP_RTYPE_VIRTMEM:
		printf("Capability resource type:\t%s\n", "Virtual Memory");
		break;
	case CAP_RTYPE_PHYSMEM:
		printf("Capability resource type:\t%s\n", "Physical Memory");
		break;
	case CAP_RTYPE_THREADPOOL:
		printf("Capability resource type:\t%s\n", "Thread Pool");
		break;
	case CAP_RTYPE_SPACEPOOL:
		printf("Capability resource type:\t%s\n", "Space Pool");
		break;
	case CAP_RTYPE_MUTEXPOOL:
		printf("Capability resource type:\t%s\n", "Mutex Pool");
		break;
	case CAP_RTYPE_MAPPOOL:
		printf("Capability resource type:\t%s\n", "Map Pool (PMDS)");
		break;
	case CAP_RTYPE_CPUPOOL:
		printf("Capability resource type:\t%s\n", "Cpu Pool");
		break;
	case CAP_RTYPE_CAPPOOL:
		printf("Capability resource type:\t%s\n", "Capability Pool");
		break;
	default:
		printf("Capability resource type:\t%s\n", "Unknown");
		break;
	}
	printf("\n");
}

/* For same space */
#define STACK_SIZE	0x1000
#define THREADS_TOTAL	10

char stack[THREADS_TOTAL][STACK_SIZE] ALIGN(8);
char *__stack_ptr = &stack[1][0];

char utcb[THREADS_TOTAL][UTCB_SIZE] ALIGN(8);
char *__utcb_ptr = &utcb[1][0];

extern void setup_new_thread(void);

int thread_create(int (*func)(void *), void *args, unsigned int flags,
		  struct task_ids *new_ids)
{
	struct task_ids ids;
	struct exregs_data exregs;
	int err;

	l4_getid(&ids);

	/* Shared space only */
	if (!(TC_SHARE_SPACE & flags)) {
		printf("%s: This function allows only "
		       "shared space thread creation.\n",
		       __FUNCTION__);
		return -EINVAL;
	}

	/* Create thread */
	if ((err = l4_thread_control(THREAD_CREATE | flags, &ids)) < 0)
		return err;

	/* Check if more stack/utcb available */
	if ((unsigned long)__utcb_ptr ==
	    (unsigned long)&utcb[THREADS_TOTAL][0])
		return -ENOMEM;
	if ((unsigned long)__stack_ptr ==
	    (unsigned long)&stack[THREADS_TOTAL][0])
		return -ENOMEM;

	/* First word of new stack is arg */
	((unsigned long *)__stack_ptr)[-1] = (unsigned long)args;

	/* Second word of new stack is function address */
	((unsigned long *)__stack_ptr)[-2] = (unsigned long)func;

	/* Setup new thread pc, sp, utcb */
	memset(&exregs, 0, sizeof(exregs));
	exregs_set_stack(&exregs, (unsigned long)__stack_ptr);
	exregs_set_utcb(&exregs, (unsigned long)__utcb_ptr);
	exregs_set_pc(&exregs, (unsigned long)setup_new_thread);

	if ((err = l4_exchange_registers(&exregs, ids.tid)) < 0)
		return err;

	/* Update utcb, stack pointers */
	__stack_ptr += STACK_SIZE;
	__utcb_ptr += UTCB_SIZE;

	/* Start the new thread */
	if ((err = l4_thread_control(THREAD_RUN, &ids)) < 0)
		return err;

	memcpy(new_ids, &ids, sizeof(ids));

	return 0;
}

int read_pager_capabilities()
{
	int ncaps;
	int err;

	/* Read number of capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_NCAPS,
					 0, &ncaps)) < 0) {
		printf("l4_capability_control() reading # of"
		       " capabilities failed.\n Could not "
		       "complete CAP_CONTROL_NCAPS request.\n");
		BUG();
	}
	total_caps = ncaps;

	/* Read all capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_READ,
					 0, cap_array)) < 0) {
		printf("l4_capability resource_control() reading of "
		       "capabilities failed.\n Could not "
		       "complete CAP_CONTROL_READ_CAPS request.\n");
		BUG();
	}

	for (int i = 0; i < total_caps; i++)
		print_capability(&cap_array[i]);

	return 0;
}

int result = 1;

int simple_pager_thread(void *arg)
{
	int err;
	int res = *(int *)arg;
	struct task_ids ids;
	int testres;

	l4_getid(&ids);

	//printf("Thread spawned from pager, "
	//       "trying to create new thread.\n");
	err = l4_thread_control(THREAD_CREATE |
				TC_SHARE_SPACE |
				TC_AS_PAGER, &ids);

	if (res == 0)
		if (err == -ENOCAP ||
		    err == -ENOMEM) {
			//printf("Creation failed with %d "
			//       "as expected.\n", err);
			testres = 0;
		} else {
			printf("Creation was supposed to fail "
			       "with %d or %d, but err = %d\n",
			       -ENOMEM, -ENOCAP, err);
			testres = -1;
		}
	else
		if (err == 0) {
			// printf("Creation succeeded as expected.\n");
			testres = 0;
		} else {
			printf("Creation was supposed to succeed, "
			       "but err = %d\n", err);
			testres = -1;
		}

	/* Destroy thread we created */
	if (err == 0 &&
	    res == 0)
		l4_thread_control(THREAD_DESTROY, &ids);

	result = testres;

	/* Destroy self */
	l4_getid(&ids);
	l4_thread_control(THREAD_DESTROY, &ids);

	return 0;
}

int wait_check_test(void)
{
	/* Wait for thread to finish */
	while (result > 0)
		;
		//l4_thread_switch(0);

	if (result != 0) {
		printf("Top-level test has failed\n");
		return -1;
	}
	result = 1;
	return 0;
}

int main(void)
{
	int err;
	struct task_ids ids;
	int TEST_MUST_FAIL = 0;
	int TEST_MUST_SUCCEED = 1;

	printf("%s: Container %s started\n",
	       __CONTAINER__, __CONTAINER_NAME__);

	/* Read pager capabilities */
	read_pager_capabilities();

	/*
	 * Create new thread that will attempt
	 * a pager privileged operation
	 */
	if ((err = thread_create(simple_pager_thread,
				 &TEST_MUST_FAIL,
				 TC_SHARE_SPACE |
				 TC_AS_PAGER, &ids)) < 0) {
		printf("Top-level simple_pager creation failed.\n");
		goto out_err;
	}

	/* Wait for test to finish and check result */
	if (wait_check_test() < 0)
		goto out_err;

#if 0
	/* Destroy test thread */
	if ((err = l4_thread_control(THREAD_DESTROY, &ids)) < 0) {
		printf("Destruction of top-level simple_pager failed.\n");
		BUG();
	}
#endif

	/*
	 * Share operations with the same thread
	 * group
	 */
	if ((err = l4_capability_control(CAP_CONTROL_SHARE,
					 CAP_SHARE_GROUP, 0)) < 0) {
		printf("Sharing capability with thread group failed.\n");
		goto out_err;
	}

	/*
	 * Create new thread that will attempt a pager privileged
	 * operation. This should succeed as we shared caps with
	 * the thread group.
	 */
	if ((err = thread_create(simple_pager_thread,
				 &TEST_MUST_SUCCEED,
				 TC_SHARE_SPACE |
				 TC_SHARE_GROUP, &ids)) < 0) {
		printf("Top-level simple_pager creation failed.\n");
		goto out_err;
	}

	/* Wait for test to finish and check result */
	if (wait_check_test() < 0)
		goto out_err;

#if 0
	/* Destroy test thread */
	if ((err = l4_thread_control(THREAD_DESTROY, &ids)) < 0) {
		printf("Destruction of top-level simple_pager failed.\n");
		BUG();
	}
#endif

	printf("Capability Sharing Test		-- PASSED --\n");

	return 0;

out_err:
	printf("Capability Sharing Test		-- FAILED --\n");
	return 0;
}
