#define _USE_XOPEN
#include <string.h>
#include <stdlib.h>

char *
strdup(const char *s)
{
	int len = strlen(s);
	char *d;
	d = malloc(len);
	if (d == NULL)
		return NULL;
	strcpy(d, s);
	return d;
}
