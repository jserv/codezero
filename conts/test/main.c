/*
 * Main function for this container
 */
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4/api/space.h>

extern int print_hello_world(void);

int main(void)
{
	print_hello_world();

	return 0;
}

