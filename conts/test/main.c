/*
 * Main function for all tests
 */
#include <l4/api/errno.h>
#include <container.h>
#include <capability.h>
#include <thread.h>
#include <tests.h>

int main(void)
{

	printf("%s: Container %s started\n",
	       __CONTAINER__, __CONTAINER_NAME__);

	capability_test();

	return 0;
}

