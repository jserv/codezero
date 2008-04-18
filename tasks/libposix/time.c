
#include <l4lib/arch/syscalls.h>
#include <sys/time.h>
#include <errno.h>

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	int ret = l4_time(tv, 0);

	/* If error, return positive error code */
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	/* else return value */
	return ret;
}
