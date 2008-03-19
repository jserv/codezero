#include <errno.h>

int errno_variable;

int *__errno_location(void)
{
	return &errno_variable;
}
