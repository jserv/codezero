/*
 * Capability-related userspace helpers
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#include <capability.h>
#include <stdio.h>
#include <l4lib/arch/syscalls.h>

static struct capability cap_array[30];

struct cap_group {
	struct cap_list virtmem;
	struct cap_list physmem;
	struct cap_list threadpool;
	struct cap_list tctrl;
	struct cap_list exregs;
	struct cap_list ipc;
	struct cap_list mutex;
	struct cap_list sched;
	struct cap_list mutexpool;
	struct cap_list spacepool;
	struct cap_list cappool;
};

static inline struct capability *cap_get_thread()
{

}

static inline struct capability *cap_get_space()
{

}

static inline struct capability *cap_get_ipc()
{

}

static inline struct capability *cap_get_virtmem()
{

}

static inline struct capability *cap_get_physmem()
{

}

static inline struct capability *cap_get_physmem(unsigned long phys)
{

}

static inline struct capability *cap_get_virtmem(unsigned long virt)
{

}

static inline struct capability *cap_get_byid(l4id_t id)
{

}


void cap_share_single(struct capability *orig, struct capability *share, l4id_t target, unsigned int flags)
{

}

void cap_grant_single(struct capability *orig, struct capability *share, l4id_t target, unsigned int flags)
{
}


void cap_print(struct capability *cap)
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

int cap_read_all(void)
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

	/* Read all capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_READ,
					 0, cap_array)) < 0) {
		printf("l4_capability resource_control() reading of "
		       "capabilities failed.\n Could not "
		       "complete CAP_CONTROL_READ_CAPS request.\n");
		BUG();
	}

	for (int i = 0; i < ncaps; i++)
		cap_print(&cap_array[i]);

	return 0;
}

