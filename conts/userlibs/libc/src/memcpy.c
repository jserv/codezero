/*
 * Copyright B Labs(R) Ltd.
 * Author: Prem Mallappa < prem.mallappa@b-labs.co.uk >
 * Generic memcpy, fairly optimized
 */

void
__attribute__ ((weak))
*memcpy(void *dst, void *src, size_t len)
{
	unsigned char *s=src, *d=dst;
	unsigned remain = 0;
	unsigned align_mask = sizeof(unsigned long) - 1;
	unsigned alignment = ((unsigned long)s & align_mask) |
				((unsigned long)d & align_mask);

	remain = len & align_mask;

	if (alignment == 0) {
			unsigned long *sl = (unsigned long*)s, *dl = (unsigned long*)d;
			while(len > remain) {
				*dl++ = *sl++;
				len -= sizeof(unsigned long);
			}
			s = (unsigned char *)sl; d = (unsigned char *)dl;
	}
	else if (alignment == 2) {
			unsigned short *sh = (unsigned short *)s, *dh = (unsigned short *)d;
			while (len > remain) {
				*dh++ = *sh++;
				len -= sizeof(unsigned short);
			}
			s = (unsigned char *)sh; d = (unsigned char *)dh;
	}
	else {
		while (len--)
			*d++ = *s++;
	}

	switch(remain) {
		case 3:
			*d++ = *s++;
		case 2:
			*d++ = *s++;
		case 1:
			*d++ = *s++;
		default: break;
	}

	return dst;
}

#if 0
/* TEST */

int memcmp1(void *src, void *dst, size_t len)
{
	char *s = src, *d = dst;
	int i;
	for(i=0; i < len; i++) {
		printf("%d %x %x\n", i, s[i], d[i]);
		if(s[i] != d[i])
			break;
	}

	if (i!=len) {
		printf("diff at %d, src: %x, dst: %x\n", i, s[i], d[i]);
		return 1;
	}
	return 0;
}

int main(void)
{
	char s[100]__attribute__((aligned (8))), d[100]__attribute__((aligned (8)));
	int i;
/* TEST 1 */
	for (i = 0; i < sizeof(s); i++) {
		s[i] = i;
		d[i] = 0;
	}

	memcpy1(&s[0], &d[0], 3);
	if(memcmp(&s[0], &d[0], 3))
		memcmp1(&s[0], &d[0], 3);

/* TEST 2 */
	for (i = 0; i < sizeof(s); i++) {
		s[i] = i;
		d[i] = 0;
	}

	memcpy1(&s[1], &d[0], 6);
	if(memcmp(&s[1], &d[0], 6))
		memcmp1(&s[1], &d[0], 6);

/* TEST 3 */
	for (i = 0; i < sizeof(s); i++) {
		s[i] = i;
		d[i] = 0;
	}
	memcpy1(&s[2], &d[1], 7);
	if(memcmp(&s[2], &d[1], 7))
		memcmp1(&s[2], &d[1], 7);

/* TEST 4 */
	for (i = 0; i < sizeof(s); i++) {
		s[i] = i;
		d[i] = 0;
	}
	memcpy1(&s[2], &d[3], 7);
	if(memcmp(&s[2], &d[3], 7))
		memcmp1(&s[2], &d[3], 7);

	return 0;
}
#endif
