/*
 * Keyboard and Mouse service for userspace
 */
#include <l4lib/lib/addr.h>
#include <l4lib/lib/thread.h>
#include <l4lib/lib/cap.h>
#include <l4lib/irq.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/errno.h>
#include <l4/api/irq.h>
#include <l4/api/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/api/space.h>
#include <mem/malloc.h>
#include <container.h>
#include <linker.h>
#include <keyboard.h>
#include <mouse.h>
#include <dev/platform.h>

#define KEYBOARDS_TOTAL		1
#define MOUSE_TOTAL         	1

static struct capability *caparray;
static int total_caps = 0;

struct keyboard kbd[KEYBOARDS_TOTAL];
struct mouse mouse[MOUSE_TOTAL];

int cap_share_all_with_space()
{
	int err;

	/* Share all capabilities */
	if ((err = l4_capability_control(CAP_CONTROL_SHARE,
					 CAP_SHARE_ALL_SPACE, 0)) < 0) {
		printf("l4_capability_control() sharing of "
		       "capabilities failed.\n Could not "
		       "complete CAP_CONTROL_SHARE request. err=%d\n",
		       err);
		BUG();
	}

	return 0;
}

int keyboard_irq_handler(void *arg)
{
	int err;
	struct keyboard *keyboard = (struct keyboard *)arg;
	const int slot = 0;

	/*
	 * For versatile, KMI refernce clock = 24MHz
	 * KMI manual says we need 8MHz clock,
	 * so divide by 3
	 */
	kmi_keyboard_init(keyboard->base, 3);
	printf("%s: Keyboard initialization done..\n", __CONTAINER_NAME__);

	/* Register self for timer irq, using notify slot 0 */
	if ((err = l4_irq_control(IRQ_CONTROL_REGISTER, slot,
				  keyboard->irq_no)) < 0) {
		printf("%s: FATAL: Keyboard irq could not be registered. "
		       "err=%d\n", __FUNCTION__, err);
		BUG();
	}

	/* Handle irqs forever */
	while (1) {
		char c;

		/* Block on irq */
		int data = l4_irq_wait(slot, keyboard->irq_no);
		while (data--)
			if ((c = kmi_keyboard_read(keyboard->base, &keyboard->state)))
				printf("%c", c);

		/*
		 * Kernel has disabled irq for keyboard
		 * We need to enable it
		 */
		kmi_rx_irq_enable(keyboard->base);
	}
}

int mouse_irq_handler(void *arg)
{
	int err;
	struct mouse *mouse = (struct mouse *)arg;
	const int slot = 0;

	/*
	 * For versatile, KMI refernce clock = 24MHz
	 * KMI manual says we need 8MHz clock,
	 * so divide by 3
	 */
	kmi_mouse_init(mouse->base, 3);
	printf("%s: Mouse initialization done..\n", __CONTAINER_NAME__);

	/* Register self for timer irq, using notify slot 0 */
	if ((err = l4_irq_control(IRQ_CONTROL_REGISTER, slot,
				  mouse->irq_no)) < 0) {
		printf("%s: FATAL: Mouse irq could not be registered. "
		       "err=%d\n", __FUNCTION__, err);
		BUG();
	}

	/* Handle irqs forever */
	while (1) {
		int c;

		/* Block on irq */
		int data = l4_irq_wait(slot, mouse->irq_no);
		while (data--)
			if ((c = kmi_data_read(mouse->base)))
				printf("mouse data: %d\n", c);

		/*
		 * Kernel has disabled irq for mouse
		 * We need to enable it
		 */
		kmi_rx_irq_enable(mouse->base);
	}
}

int kmi_setup_devices(void)
{
	struct l4_thread thread;
	struct l4_thread *tptr = &thread;
	int err;

	kbd[0].phys_base = PLATFORM_KEYBOARD0_BASE;
	kbd[0].irq_no = IRQ_KEYBOARD0;

	for (int i = 0; i < KEYBOARDS_TOTAL; i++) {
		/* Get one page from address pool */
		kbd[i].base = (unsigned long)l4_new_virtual(1);
		kbd[i].state.shift = 0;
		kbd[i].state.caps_lock = 0;
		kbd[i].state.keyup = 0;

		/* Map timer to a virtual address region */
		if (IS_ERR(l4_map((void *)kbd[i].phys_base,
				  (void *)kbd[i].base, 1,
				  MAP_USR_IO, self_tid()))) {
			printf("%s: FATAL: Failed to map Keyboard device "
			       "to a virtual address\n",
			       __CONTAINER_NAME__);
			BUG();
		}

		/*
		 * Create new keyboard irq handler thread.
		 *
		 * This will initialize its keyboard argument, register
		 * itself as its irq handler, initiate keyboard and
		 * wait on irqs.
		 */
		if ((err = thread_create(keyboard_irq_handler, &kbd[i],
					 TC_SHARE_SPACE,
					 &tptr)) < 0) {
			printf("FATAL: Creation of irq handler "
			       "thread failed.\n");
			BUG();
		}
	}

	mouse[0].phys_base = PLATFORM_MOUSE0_BASE;
	mouse[0].irq_no = IRQ_MOUSE0;

        for (int i = 0; i < MOUSE_TOTAL; i++) {
		/* Get one page from address pool */
		mouse[i].base = (unsigned long)l4_new_virtual(1);

		/* Map timer to a virtual address region */
		if (IS_ERR(l4_map((void *)mouse[i].phys_base,
			   (void *)mouse[i].base, 1,
			   MAP_USR_IO, self_tid()))) {
			printf("%s: FATAL: Failed to map Mouse device "
			       "to a virtual address\n",
			       __CONTAINER_NAME__);
			BUG();
		}

		/*
		 * Create new mouse irq handler thread.
		 *
		 * This will initialize its mouse argument, register
		 * itself as its irq handler, initiate mouse and
		 * wait on irqs.
		 */
		if ((err = thread_create(mouse_irq_handler, &mouse[i],
					 TC_SHARE_SPACE, &tptr)) < 0) {
			printf("FATAL: Creation of irq handler "
			       "thread failed.\n");
			BUG();
		}
	}

	return 0;
}

/*
 * Declare a statically allocated char buffer
 * with enough bitmap size to cover given size
 */
#define DECLARE_IDPOOL(name, size)      \
char name[(sizeof(struct id_pool) + ((size >> 12) >> 3))]

#define PAGE_POOL_SIZE                  SZ_1MB
static struct address_pool device_vaddr_pool;
DECLARE_IDPOOL(device_id_pool, PAGE_POOL_SIZE);

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
			if (__pfn(page_align_up(__end)) + KEYBOARDS_TOTAL +
			    MOUSE_TOTAL <= caparray[i].end) {
				/*
				 * Yes. We initialize the device
				 * virtual memory pool here.
				 *
				 * We may allocate virtual memory
				 * addresses from this pool.
				 */
				address_pool_init(&device_vaddr_pool,
						  (struct id_pool *)&device_id_pool,
						  page_align_up(__end),
						  __pfn_to_addr(caparray[i].end));
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

void main(void)
{
	/* Read all capabilities */
	caps_read_all();

	total_caps = cap_get_count();
	caparray = cap_get_all();

	/* Share all with space */
	cap_share_all_with_space();

	/* Initialize virtual address pool for timers */
	init_vaddr_pool();

	/* Map and initialize keyboard devices */
	kmi_setup_devices();

	/* Listen for timer requests */
	while (1)
		handle_requests();
}


