#include <string.h>

int strlen(const char *s)
{
	const char *p;

	for (p = s; *p != '\0'; p++);
	return p - s;
}

char *strcpy(char *to, const char *from)
{
	char *t = to;

	while ((*to++ = *from++) != '\0')
		;
	return t;
}

void *memset(void *p, int c, int size)
{
	char ch;
	char *pp;

	pp = (char *)p;
	ch = (char)c;

	for (int i = 0; i < size; i++) {
		*pp++ = ch;
	}
	return p;
}

void *memcpy(void *d, void *s, int size)
{
	char *dst = (char *)d;
	char *src = (char *)s;

	for (int i = 0; i < size; i++) {
		*dst = *src;
		dst++;
		src++;
	}
	return d;
}


int strcmp(const char *s1, const char *s2)
{
	unsigned int i = 0;
	int d;

	while(1) {
		d = (unsigned char)s1[i] - (unsigned char)s2[i];
		if (d != 0 || s1[i] == '\0')
			return d;
		i++;
	}
}

/*
 * Copies string pointed by @from to string pointed by @to.
 *
 * If count is greater than the length of string in @from,
 * pads rest of the locations with null.
 */
char *strncpy(char *to, const char *from, int count)
{
	char *temp = to;

	while (count) {
		*temp = *from;

		/*
		 * Stop updating from if null
		 * terminator is reached.
		 */
		if (*from)
			from++;
		temp++;
		count--;
	}
	return to;
}
