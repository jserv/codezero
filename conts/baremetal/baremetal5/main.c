/*
 * Main function for this container
 */
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4/api/space.h>
#include <l4lib/ipcdefs.h>
#include <l4/api/errno.h>
#include "sp804_timer.h"

/*
 * Address where we want  to map timer
 * Be sure that we using a valid address, depending upon
 * how we make use of this driver.
 */
#define TIMER_VIRT_BASE	0x91000000

/* Frequency of timer in MHz */
#define TIMER_FREQUENCY	1

#define __TASKNAME__	"Driver Mapper"

void handle_request(void)
{
# if 0
	/* Generic ipc data */
	u32 mr[MR_UNUSED_TOTAL];
	l4id_t senderid;
	u32 tag;
	int ret;

		if ((ret = l4_receive(L4_ANYTHREAD)) < 0) {
			printf("%s: %s: IPC Error: %d. Quitting...\n", __TASKNAME__,
				   __FUNCTION__, ret);
			BUG();
		}

		/* Syslib conventional ipc data which uses first few mrs. */
		tag = l4_get_tag();
		senderid = l4_get_sender();


		if (!(sender = find_task(senderid))) {
			l4_ipc_return(-ESRCH);
			return;
		}


		/* Read mrs not used by syslib */
		for (int i = 0; i < MR_UNUSED_TOTAL; i++)
			mr[i] = read_mr(MR_UNUSED_START + i);

		switch(tag) {
			case L4_IPC_TAG_GET_TIME:
				ret = sp804_read_value(TIMER_VIRT_BASE);
			break;
		}

		/* Reply */
		if ((ret = l4_ipc_return(ret)) < 0) {
			printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, ret);
			BUG();
		}
#endif	
}

int main(void)
{
	int ret = 0;
	unsigned value = 0;

	/*
	 * Map Timer 2 to Ram allocated to us
	 * FIXME:  do we need to set the frequency for sp810?
	 */
	ret = l4_map((void *)TIMER2_PHYS_BASE, (void *)TIMER_VIRT_BASE, 1, \
					      MAP_USR_IO_FLAGS, self_tid());
	if (ret) {
		printf("Failed to map the requested device\n");
		return ret;
	}

	/* Initialise timer */
	sp804_init(TIMER_VIRT_BASE, SP804_TIMER_RUNMODE_FREERUN, \
		   SP804_TIMER_WRAPMODE_WRAPPING, SP804_TIMER_WIDTH32BIT, \
		   SP804_TIMER_IRQDISABLE);

	sp804_enable(TIMER_VIRT_BASE, 1);

#if 1
	/* Read Timer value */
	while(1) {
		value = sp804_read_value(TIMER_VIRT_BASE);
		printf("Read timer with value = %x\n", value);
	}
#else
	printf("Driver Mapper: Waiting for ipc\n");
	while (1) {
		handle_request();
	}
#endif

	return ret;
}

