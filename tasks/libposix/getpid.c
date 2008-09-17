
#include <l4lib/arch/syscalls.h>
#include <l4lib/arch/syslib.h>
#include <sys/types.h>
#include <unistd.h>

pid_t getpid(void)
{
	return self_tid();
}
