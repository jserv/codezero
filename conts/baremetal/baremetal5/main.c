/*
 * Timer service for userspace
 */
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/addr.h>
#include <l4lib/exregs.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/errno.h>

#include <l4/api/space.h>
#include <capability.h>
#include <container.h>
#include "sp804_timer.h"
#include <linker.h>


/* Frequency of timer in MHz */
#define TIMER_FREQUENCY	1

#define TIMERS_TOTAL		1

static struct capability caparray[32];
static int total_caps = 0;

struct capability timer_cap[TIMERS_TOTAL];


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
		printf("Capability resource type:\t%s\n", "IRQ");
		break;
	}
}

void cap_array_print()
{
	printf("Capabilities\n"
	       "~~~~~~~~~~~~\n");

	for (int i = 0; i < total_caps; i++)
		cap_print(&caparray[i]);

	printf("\n");
}

int cap_read_all()
{
	int ncaps;
	int err;

	/* Read number of capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_NCAPS,
					 0, 0, 0, &ncaps)) < 0) {
		printf("l4_capability_control() reading # of"
		       " capabilities failed.\n Could not "
		       "complete CAP_CONTROL_NCAPS request.\n");
		BUG();
	}
	total_caps = ncaps;

	/* Read all capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_READ,
					 0, 0, 0, caparray)) < 0) {
		printf("l4_capability_control() reading of "
		       "capabilities failed.\n Could not "
		       "complete CAP_CONTROL_READ_CAPS request.\n");
		BUG();
	}
#if 0
	cap_array_print(&caparray);
#endif
	return 0;
}

/*
 * Scans for up to TIMERS_TOTAL timer devices in capabilities.
 */
int timer_probe_devices(void)
{
	int timers = 0;

	/* Scan for timer devices */
	for (int i = 0; i < total_caps; i++) {
		/* Match device type */
		if (cap_devtype(&caparray[i]) == CAP_DEVTYPE_TIMER) {
			/* Copy to correct device index */
			memcpy(&timer_cap[cap_devnum(&caparray[i]) - 1],
			       &caparray[i], sizeof(timer_cap[0]));
			timers++;
		}
	}

	if (timers != TIMERS_TOTAL) {
		printf("%s: Error, not all timers could be found. "
		       "timers=%d\n", __CONTAINER_NAME__, timers);
		return -ENODEV;
	}
	return 0;
}

static struct sp804_timer timer[TIMERS_TOTAL];

int timer_setup_devices(void)
{
	for (int i = 0; i < TIMERS_TOTAL; i++) {
		/* Get one page from address pool */
		timer[i].base = (unsigned long)l4_new_virtual(1);
		
		/* Map timers to a virtual address region */
		if (IS_ERR(l4_map((void *)__pfn_to_addr(timer_cap[i].start),
				  	(void *)timer[i].base, timer_cap[i].size, MAP_USR_IO_FLAGS,
				  	self_tid()))) {
			printf("%s: FATAL: Failed to map TIMER device "
			       "%d to a virtual address\n",
			       __CONTAINER_NAME__,
			       cap_devnum(&timer_cap[i]));
			BUG();
		}

		/* Initialise timer */
		sp804_init(timer[i].base, SP804_TIMER_RUNMODE_FREERUN, \
			SP804_TIMER_WRAPMODE_WRAPPING, SP804_TIMER_WIDTH32BIT, \
			SP804_TIMER_IRQDISABLE);

		/* Enable Timer */		
		sp804_enable(timer[i].base, 1);
	}
	return 0;
}

static struct address_pool device_vaddr_pool;

/*
 * Initialize a virtual address pool
 * for mapping physical devices.
 
 */
void init_vaddr_pool(void)
{
	for (int i = 0; i < total_caps; i++) {
		/* Find the virtual memory region for this process */
		if (cap_type(&caparray[i]) == CAP_TYPE_MAP_VIRTMEM &&
		    __pfn_to_addr(caparray[i].start) ==
		    (unsigned long)vma_start) {

			/*
			 * Do we have any unused virtual space
			 * where we run, and do we have enough
			 * pages of it to map all timers?
			 */
			if (__pfn(page_align_up(__end))
			    + TIMERS_TOTAL <= caparray[i].end) {
				/*
				 * Yes. We initialize the device
				 * virtual memory pool here.
				 *
				 * We may allocate virtual memory
				 * addresses from this pool.
				 */
				address_pool_init(&device_vaddr_pool, page_align_up(__end),
					__pfn_to_addr(caparray[i].end), TIMERS_TOTAL);
				return;
			} else
				goto out_err;
		}
	}

out_err:
	printf("%s: FATAL: No virtual memory "
	       "region available to map "
	       "devices.\n", __CONTAINER_NAME__);
	BUG();
}

void *l4_new_virtual(int npages)
{
	
	return address_new(&device_vaddr_pool, npages, PAGE_SIZE);
}

int timer_gettime(int devno)
{
	return sp804_read_value(timer[devno].base);
}

void handle_requests(void)
{
	l4id_t senderid;
	u32 tag;
	int ret;

	printf("%s: Initiating ipc.\n", __CONTAINER__);
	if ((ret = l4_receive(L4_ANYTHREAD)) < 0) {
		printf("%s: %s: IPC Error: %d. Quitting...\n", __CONTAINER__,
		       __FUNCTION__, ret);
		BUG();
	}

	/* Syslib conventional ipc data which uses first few mrs. */
	tag = l4_get_tag();
	senderid = l4_get_sender();
	
	/*
	 * TODO:
	 *
	 * Maybe add tags here that handle requests for sharing
	 * of the requested timer device with the client?
	 *
	 * In order to be able to do that, we should have a
	 * shareable/grantable capability to the device. Also
	 * the request should (currently) come from a task
	 * inside the current container
	 */
	switch (tag) {
	case L4_IPC_TAG_TIMER_GETTIME:
		timer_gettime(1);
		break;

	default:
		printf("%s: Error received ipc from 0x%x residing "
		       "in container %x with an unrecognized tag: "
		       "0x%x\n", __CONTAINER__, senderid,
		       __cid(senderid), tag);
	}

	/* Reply */
	if ((ret = l4_ipc_return(ret)) < 0) {
		printf("%s: IPC return error: %d.\n", __FUNCTION__, ret);
		BUG();
	}
}

/*
 * UTCB-size aligned utcb.
 *
 * BIG WARNING NOTE: This declaration is legal if we are
 * running in a disjoint virtual address space, where the
 * utcb declaration lies in a unique virtual address in
 * the system.
 */
#define DECLARE_UTCB(name) \
	struct utcb name ALIGN(sizeof(struct utcb))

DECLARE_UTCB(utcb);

/* Set up own utcb for ipc */
int l4_utcb_setup(void *utcb_address)
{
	struct task_ids ids;
	struct exregs_data exregs;
	int err;

	l4_getid(&ids);

	/* Clear utcb */
	memset(utcb_address, 0, sizeof(struct utcb));

	/* Setup exregs for utcb request */
	memset(&exregs, 0, sizeof(exregs));
	exregs_set_utcb(&exregs, (unsigned long)utcb_address);

	if ((err = l4_exchange_registers(&exregs, ids.tid)) < 0)
		return err;

	return 0;
}

void main(void)
{
	int err;

	/* Read all capabilities */
	cap_read_all();

	/* Scan for timer devices in capabilities */
	timer_probe_devices();

	/* Initialize virtual address pool for timers */
	init_vaddr_pool();

	/* Map and initialize timer devices */
	timer_setup_devices();

	/* Setup own utcb */
	if ((err = l4_utcb_setup(&utcb)) < 0) {
		printf("FATAL: Could not set up own utcb. "
		       "err=%d\n", err);
		BUG();
	}

	/* Listen for timer requests */
	while (1)
		handle_requests();
}


