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
#include <malloc/malloc.h>
#include <l4lib/capability/cap_print.h>
#include <container.h>
#include "sp804_timer.h"
#include <linker.h>
#include <timer.h>

/* Frequency of timer in MHz */
#define TIMER_FREQUENCY		1

#define TIMERS_TOTAL		1

static struct capability caparray[32];
static int total_caps = 0;

static struct timer timer[TIMERS_TOTAL];
static int notify_slot = 0;

int cap_read_all()
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
					 0, caparray)) < 0) {
		printf("l4_capability_control() reading of "
		       "capabilities failed.\n Could not "
		       "complete CAP_CONTROL_READ_CAPS request.\n");
		BUG();
	}
	//cap_array_print(total_caps, caparray);

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
			memcpy(&timer[cap_devnum(&caparray[i]) - 1].cap,
			       &caparray[i], sizeof(timer[0].cap));
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

void timer_irq_handler(void *arg)
{
	struct timer *timer = (struct timer *)arg;

	/* Initialise timer */
	sp804_init(timer->base, SP804_TIMER_RUNMODE_PERIODIC,
		   SP804_TIMER_WRAPMODE_WRAPPING,
		   SP804_TIMER_WIDTH32BIT, SP804_TIMER_IRQENABLE);

	/* Register self for timer irq, using notify slot 0 */
	if ((err = l4_irq_control(IRQ_CONTROL_REGISTER, 0,
				  timer->cap.irqnum)) < 0) {
		printf("%s: FATAL: Timer irq could not be registered. "
		       "err=%d\n", __FUNCTION__, err);
		BUG();
	}

	/* Enable Timer */
	sp804_enable(timer->base, 1);

	/* Handle irqs forever */
	while (1) {
		int count;

		/* Block on irq */
		count = l4_irq_wait(timer->cap.irqnum);

		/* Update timer count */
		timer->count += count;

		/* Print both counter and number of updates on count */
		printf("Timer count: %lld, current update: %d\n",
		       timer->count, count);
	}
}

int timer_setup_devices(void)
{
	struct task_ids irq_tids;

	for (int i = 0; i < TIMERS_TOTAL; i++) {
		/* Get one page from address pool */
		timer[i].base = (unsigned long)l4_new_virtual(1);
		timer[i].count = 0;
		link_init(&timer[i].tasklist);
		l4_mutex_init(&timer[i].lock);

		/* Map timer to a virtual address region */
		if (IS_ERR(l4_map((void *)__pfn_to_addr(timer[i].cap.start),
				  (void *)timer[i].base, timer[i].cap.size,
				  MAP_USR_IO_FLAGS,
				  self_tid()))) {
			printf("%s: FATAL: Failed to map TIMER device "
			       "%d to a virtual address\n",
			       __CONTAINER_NAME__,
			       cap_devnum(&timer_cap[i]));
			BUG();
		}

		/*
		 * Create new timer irq handler thread.
		 *
		 * This will initialize its timer argument, register
		 * itself as its irq handler, initiate the timer and
		 * wait on irqs.
		 */
		if ((err = thread_create(timer_irq_handler, &timer[i],
					 TC_SHARED_SPACE,
					 &irq_tids)) < 0) {
			printf("FATAL: Creation of irq handler "
			       "thread failed.\n");
			BUG();
		}
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
		if (cap_type(&caparray[i]) == CAP_TYPE_MAP_VIRTMEM
		    && __pfn_to_addr(caparray[i].start) ==
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
				address_pool_init(&device_vaddr_pool,
						  page_align_up(__end),
						  __pfn_to_addr(caparray[i].end),
						  TIMERS_TOTAL);
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

#if 0
struct timer_task *get_timer_task(l4id_t tgid)
{
	/* May be we can prepare a cache for timer_task structs */
	struct timer_task *task = (struct timer_task *)kzalloc(sizeof(struct timer_task));

	link_init(&task->list);
	task->tgid = tgid;
	task->wait_count = timer[0].count;

	return task;
}

void free_timer_task(struct timer_task *task)
{
	kfree(task);
}

void timer_irq_handler(void)
{
	struct timer_task *struct_ptr, *temp_ptr;

	timer[0].count += 1;

	/*
	 * FIXME:
	 * Traverse through the sleeping process list and
	 * wake any process if required, we need to put this part in bottom half
	 */
	list_foreach_removable_struct(struct_ptr, temp_ptr, &timer[0].tasklist, list)
		if (struct_ptr->wait_count == timer[0].count) {

			/* Remove task from list */
			l4_mutex_lock(&timer[0].lock);
			list_remove(&struct_ptr->list);
			l4_mutex_unlock(&timer[0].lock);

			/* wake the sleeping process, send wake ipc */

			free_timer_task(struct_ptr);
		}
}

int timer_gettime(void)
{
	return timer[0].count;
}

void timer_sleep(l4id_t tgid, int sec)
{
	struct timer_task *task = get_timer_task(tgid);

	/* Check for overflow */
	task->wait_count += (sec * 1000000);

	l4_mutex_lock(&timer[0].lock);
	list_insert_tail(&task->list, &timer[0].tasklist);
	l4_mutex_unlock(&timer[0].lock);
}

#endif

void handle_requests(void)
{
	u32 mr[MR_UNUSED_TOTAL];
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

	/* Read mrs not used by syslib */
	for (int i = 0; i < MR_UNUSED_TOTAL; i++)
		mr[i] = read_mr(MR_UNUSED_START + i);

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
		//mr[0] = timer_gettime();
		break;

	case L4_IPC_TAG_TIMER_SLEEP:
		//timer_sleep(senderid, mr[0]);
		/* TODO: Halt the caller for mr[0] seconds */
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

	/* Setup own static utcb */
	if ((err = l4_utcb_setup(&utcb)) < 0) {
		printf("FATAL: Could not set up own utcb. "
		       "err=%d\n", err);
		BUG();
	}

	/* Map and initialize timer devices */
	timer_setup_devices();

	/* Listen for timer requests */
	while (1)
		handle_requests();
}


