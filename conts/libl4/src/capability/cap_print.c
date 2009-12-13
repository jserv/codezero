

/* Capability printing generic routines */

#include <l4lib/capability/cap_print.h>
#include <stdio.h>

void cap_dev_print(struct capability *cap)
{
	switch (cap_devtype(cap)) {
	case CAP_DEVTYPE_UART:
		printf("Device type:\t\t\t%s%d\n", "UART", cap_devnum(cap));
		break;
	case CAP_DEVTYPE_TIMER:
		printf("Device type:\t\t\t%s%d\n", "Timer", cap_devnum(cap));
		break;
	case CAP_DEVTYPE_CLCD:
		printf("Device type:\t\t\t%s%d\n", "CLCD", cap_devnum(cap));
		break;
	default:
		return;
	}
	printf("Device Irq:\t\t%d\n", cap->irq);
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
	case CAP_TYPE_MAP_PHYSMEM:
		if (!cap_is_devmem(cap)) {
			printf("Capability type:\t\t%s\n", "Map/Physmem");
		} else {
			printf("Capability type:\t\t%s\n", "Map/Physmem/Device");
			cap_dev_print(cap);
		}
		break;
	case CAP_TYPE_MAP_VIRTMEM:
		printf("Capability type:\t\t%s\n", "Map/Virtmem");
		break;
	case CAP_TYPE_IPC:
		printf("Capability type:\t\t%s\n", "Ipc");
		break;
	case CAP_TYPE_UMUTEX:
		printf("Capability type:\t\t%s\n", "Mutex");
		break;
	case CAP_TYPE_IRQCTRL:
		printf("Capability type:\t\t%s\n", "IRQ Control");
		break;
	case CAP_TYPE_QUANTITY:
		printf("Capability type:\t\t%s\n", "Quantitative");
		break;
	case CAP_TYPE_CAP:
		printf("Capability type:\t\t%s\n", "Capability Control");
		break;
	default:
		printf("Capability type:\t\t%s, cap_type=0x%x\n",
		       "Unknown", cap_type(cap));
		break;
	}

	switch (cap_rtype(cap)) {
	case CAP_RTYPE_THREAD:
		printf("Capability resource type:\t%s\n", "Thread");
		break;
	case CAP_RTYPE_SPACE:
		printf("Capability resource type:\t%s\n", "Space");
		break;
	case CAP_RTYPE_CONTAINER:
		printf("Capability resource type:\t%s\n", "Container");
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
		printf("Capability resource type:\t%s, id=0x%x\n", "Unknown",
		       cap_rtype(cap));
		break;
	}
	printf("\n");
}

void cap_array_print(int total_caps, struct capability *caparray)
{
	printf("Capabilities\n"
	       "~~~~~~~~~~~~\n");

	for (int i = 0; i < total_caps; i++)
		cap_print(&caparray[i]);

	printf("\n");
}

