/*
 * UART service for userspace
 */
#include <l4lib/macros.h>
#include L4LIB_INC_ARCH(syslib.h)
#include L4LIB_INC_ARCH(syscalls.h)
#include <l4lib/exregs.h>
#include <l4lib/lib/addr.h>
#include <l4lib/lib/cap.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/errno.h>
#include <l4/api/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/api/space.h>
#include <container.h>
#include <linker.h>
#include <uart.h>
#include <dev/uart.h>
#include <dev/platform.h>

/* Capabilities of this service */
static struct capability *caparray;
static int total_caps = 0;

/* Number of UARTS to be managed by this service */
#define UARTS_TOTAL             1
static struct uart uart[UARTS_TOTAL];

int uart_setup_devices(void)
{
	int err;

	uart[0].phys_base = PLATFORM_UART1_BASE;

	for (int i = 0; i < UARTS_TOTAL; i++) {
		/* Get one page from address pool */
		uart[i].base = (unsigned long)l4_new_virtual(1);

		/* Map uart to a virtual address region */
		if (IS_ERR(err = l4_map((void *)uart[i].phys_base,
				  (void *)uart[i].base, 1,
				  MAP_USR_IO, self_tid()))) {
			printf("%s: FATAL(%d): Failed to map UART device "
			       "0x%lx physcial to 0x%lx virtual address\n",
			       __CONTAINER_NAME__, err,
			       uart[i].phys_base, uart[i].base);
			BUG();
		}

		/* Initialize uart */
		uart_init(uart[i].base);
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
		if (cap_type(&caparray[i]) == CAP_TYPE_MAP_VIRTMEM &&
		    __pfn_to_addr(caparray[i].start) ==
		    (unsigned long)vma_start) {

			/*
			 * Do we have any unused virtual space
			 * where we run, and do we have enough
			 * pages of it to map all uarts?
			 */
			if (__pfn(page_align_up(__end))
			    + UARTS_TOTAL <= caparray[i].end) {
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

void uart_generic_tx(char c, int devno)
{
	uart_tx_char(uart[devno].base, c);
}

char uart_generic_rx(int devno)
{
	return uart_rx_char(uart[devno].base);
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
	 * of the requested uart device with the client?
	 *
	 * In order to be able to do that, we should have a
	 * shareable/grantable capability to the device. Also
	 * the request should (currently) come from a task
	 * inside the current container
	 */

	/*
	  * FIXME: Right now we are talking to UART1 by default, we need to define protocol
	  * for sommunication with UART service
	  */
	switch (tag) {
	case L4_IPC_TAG_UART_SENDCHAR:
		printf("got L4_IPC_TAG_UART_SENDCHAR with char %d\n ", mr[0]);
		uart_generic_tx((char)mr[0], 0);
		break;
	case L4_IPC_TAG_UART_RECVCHAR:
		mr[0] = (int)uart_generic_rx(0);
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

void main(void)
{
	/* Read all capabilities */
	caps_read_all();

	total_caps = cap_get_count();
	caparray = cap_get_all();

	/* Initialize virtual address pool for uarts */
	init_vaddr_pool();

	/* Map and initialize uart devices */
	uart_setup_devices();

	/* Listen for uart requests */
	while (1)
		handle_requests();
}

