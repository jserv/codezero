/*
 * Initialise system call offsets.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <l4lib/kip.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/utcb.h>
#include <l4lib/ipcdefs.h>
#include <l4/macros.h>
#include INC_GLUE(memlayout.h)
#include <stdio.h>
#include <sys/types.h>
#include <sys/shm.h>

__l4_ipc_t __l4_ipc = 0;
__l4_map_t __l4_map = 0;
__l4_unmap_t __l4_unmap = 0;
__l4_kread_t __l4_kread = 0;
__l4_getid_t __l4_getid = 0;
__l4_thread_switch_t __l4_thread_switch = 0;
__l4_thread_control_t __l4_thread_control = 0;
__l4_ipc_control_t __l4_ipc_control = 0;
__l4_space_control_t __l4_space_control = 0;
__l4_exchange_registers_t __l4_exchange_registers = 0;
__l4_kmem_grant_t __l4_kmem_grant = 0;
__l4_kmem_reclaim_t __l4_kmem_reclaim = 0;

struct kip *kip;

/*
 * Private UTCB of this task. Used only for pushing/reading ipc
 * message registers.
 */
struct utcb utcb;

/*
 * Shared utcb page for this task. Used for passing data among ipc
 * parties when message registers are not big enough. Every thread
 * has right to own one, and it has an address unique to every
 * thread. It must be explicitly mapped by both parties of the ipc
 * in order to be useful.
 */
void *utcb_page;


/*
 * Obtains a unique address for the task's shared utcb page. Note this
 * does *not* map the utcb, just returns the address. This address
 * is used as an shm key to map it via shmget()/shmat() later on.
 */
static void *l4_utcb_page(void)
{
	void *addr;
	int err;

	/* We're asking it for ourself. */
	write_mr(L4SYS_ARG0, self_tid());

	/* Call pager with utcb address request. Check ipc error. */
	if ((err = l4_sendrecv(PAGER_TID, PAGER_TID, L4_IPC_TAG_UTCB)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		return PTR_ERR(err);
	}

	/* Check if syscall itself was successful */
	if (IS_ERR(addr = (void *)l4_get_retval())) {
		printf("%s: Request UTCB Address Error: %d.\n",
		       __FUNCTION__, (int)addr);
		return addr;
	}

	return addr;
}

/*
 * Initialises a non-pager task's shared memory utcb page
 * using posix semantics.
 */
int utcb_init(void)
{
	int shmid;
	void *shmaddr;

	/*
	 * Initialise utcb only if we're not the pager.
	 * The pager does it differently for itself.
	 */
	if (self_tid() != PAGER_TID) {

		/* Obtain our utcb page address */
		utcb_page = l4_utcb_page();
		printf("%s: UTCB Read from mm0 as: 0x%x\n", __FUNCTION__,
		       (unsigned long)utcb_page);

		/* Use it as a key to create a shared memory region */
		BUG_ON((shmid = shmget((key_t)utcb_page,
				       PAGE_SIZE, IPC_CREAT)) < 0);

		/* Attach to the region */
		BUG_ON((shmaddr = shmat(shmid, utcb_page, 0)) < 0);
		BUG_ON(shmaddr != utcb_page);
	}

	return 0;
}

void __l4_init(void)
{
	kip = l4_kernel_interface(0, 0, 0);

	__l4_ipc =		(__l4_ipc_t)kip->ipc;
	__l4_map =		(__l4_map_t)kip->map;
	__l4_unmap =		(__l4_unmap_t)kip->unmap;
	__l4_kread =		(__l4_kread_t)kip->kread;
	__l4_getid =		(__l4_getid_t)kip->getid;
	__l4_thread_switch =	(__l4_thread_switch_t)kip->thread_switch;
	__l4_thread_control=	(__l4_thread_control_t)kip->thread_control;
	__l4_ipc_control=	(__l4_ipc_control_t)kip->ipc_control;
	__l4_space_control=	(__l4_space_control_t)kip->space_control;
	__l4_exchange_registers =
			(__l4_exchange_registers_t)kip->exchange_registers;
	__l4_kmem_grant =	(__l4_kmem_grant_t)kip->kmem_grant;
	__l4_kmem_reclaim =	(__l4_kmem_reclaim_t)kip->kmem_reclaim;

	utcb_init();
}

