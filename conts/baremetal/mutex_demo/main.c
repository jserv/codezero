/*
 * Main function for this container
 */

#include <l4lib/macros.h>
#include L4LIB_INC_ARCH(syslib.h)
#include L4LIB_INC_ARCH(syscalls.h)
#include <l4/api/space.h>

extern int test_api_mutexctrl(void);

int main(void)
{
	test_api_mutexctrl();

	return 0;
}

