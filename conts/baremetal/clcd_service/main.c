/*
 * CLCD service for userspace
 */
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/addr.h>
#include <l4lib/exregs.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/errno.h>

#include <l4/api/space.h>
#include <l4lib/capability/cap_print.h>
#include <container.h>
#include <pl110_clcd.h>  /* FIXME: Its best if this is <libdev/uart/pl011.h> */
#include <linker.h>

#define CLCD_TOTAL		1

static struct capability caparray[32];
static int total_caps = 0;

struct capability clcd_cap[CLCD_TOTAL];

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

	return 0;
}

/*
 * Scans for up to CLCD_TOTAL clcd devices in capabilities.
 */
int clcd_probe_devices(void)
{
	int clcds = 0;

	/* Scan for clcd devices */
	for (int i = 0; i < total_caps; i++) {
		/* Match device type */
		if (cap_devtype(&caparray[i]) == CAP_DEVTYPE_CLCD) {
			/* Copy to correct device index */
			memcpy(&clcd_cap[cap_devnum(&caparray[i])],
			       &caparray[i], sizeof(clcd_cap[0]));
			clcds++;
		}
	}

	if (clcds != CLCD_TOTAL) {
		printf("%s: Error, not all clcd could be found. "
		       "total clcds=%d\n", __CONTAINER_NAME__, clcds);
		return -ENODEV;
	}
	return 0;
}

/* 
  * 1MB frame buffer,
  * FIXME: can we do dma in this buffer?
  */
#define CLCD_FRAMEBUFFER_SZ	0x100000
static char framebuffer[CLCD_FRAMEBUFFER_SZ];

static struct pl110_clcd clcd[CLCD_TOTAL];

int clcd_setup_devices(void)
{
	for (int i = 0; i < CLCD_TOTAL; i++) {
		/* Get one page from address pool */
		clcd[i].virt_base = (unsigned long)l4_new_virtual(1);

		/* Map clcd to a virtual address region */
		if (IS_ERR(l4_map((void *)__pfn_to_addr(clcd_cap[i].start),
				  (void *)clcd[i].virt_base, clcd_cap[i].size,
				  MAP_USR_IO_FLAGS,
				  self_tid()))) {
			printf("%s: FATAL: Failed to map CLCD device "
			       "%d to a virtual address\n",
			       __CONTAINER_NAME__,
			       cap_devnum(&clcd_cap[i]));
			BUG();
		}

		/* Initialize clcd */
		pl110_initialise(&clcd[i], framebuffer);
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
			 * pages of it to map all clcd?
			 */
			if (__pfn(page_align_up(__end))
			    + CLCD_TOTAL <= caparray[i].end) {
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
						  CLCD_TOTAL);
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

void handle_requests(void)
{
	u32 mr[MR_UNUSED_TOTAL];
	l4id_t senderid;
	u32 tag;
	int ret;

	printf("%s: Initiating ipc.\n", __CONTAINER__);
	if ((ret = l4_receive(L4_ANYTHREAD)) < 0) {
		printf("%s: %s: IPC Error: %d. Quitting...\n",
		       __CONTAINER__, __FUNCTION__, ret);
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
	 * of the requested cld device with the client?
	 *
	 * In order to be able to do that, we should have a
	 * shareable/grantable capability to the device. Also
	 * the request should (currently) come from a task
	 * inside the current container
	 */

	/*
	  * FIXME: Right now we are talking to CLCD by default, we need to define protocol
	  * for sommunication with CLCD service
	  */
	switch (tag) {

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
 * BIG WARNING NOTE:
 * This in-place declaration is legal if we are running
 * in a disjoint virtual address space, where the utcb
 * declaration lies in a unique virtual address in
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

	/* Scan for clcd devices in capabilities */
	clcd_probe_devices();

	/* Initialize virtual address pool for clcds */
	init_vaddr_pool();

	/* Map and initialize clcd devices */
	clcd_setup_devices();

	/* Setup own utcb */
	if ((err = l4_utcb_setup(&utcb)) < 0) {
		printf("FATAL: Could not set up own utcb. "
		       "err=%d\n", err);
		BUG();
	}

	/* Listen for clcd requests */
	while (1)
		handle_requests();
}

