#include <errno.h>
#include <stdio.h>

int errno_variable;

void perror(const char *str)
{
	printf("%s: %d\n", str, errno);
}

int *__errno_location(void)
{
	return &errno_variable;
}
