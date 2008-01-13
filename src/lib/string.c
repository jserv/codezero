

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

/* LICENCE: Taken from linux for now BB.
 * strncpy - Copy a length-limited, %NUL-terminated string
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 * @count: The maximum number of bytes to copy
 *
 * The result is not %NUL-terminated if the source exceeds
 * @count bytes.
 *
 * In the case where the length of @src is less than  that  of
 * count, the remainder of @dest will be padded with %NUL.
 *
 */
char *strncpy(char *dest, const char *src, int count)
{
	char *tmp = dest;
 
	while (count) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		count--;
	}
	return dest;
}

