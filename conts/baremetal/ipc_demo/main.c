/*
 * Main function for this container
 */

#include <l4lib/macros.h>
#include L4LIB_INC_ARCH(syslib.h)
#include L4LIB_INC_ARCH(syscalls.h)
#include <l4/api/space.h>
#include <l4lib/lib/thread.h>
#include <l4lib/lib/cap.h>

#include <memory.h>

extern int ipc_demo(void);

int main(void)
{

	__l4_threadlib_init();

	__l4_capability_init();

	page_pool_init();

	ipc_demo();

	return 0;
}

