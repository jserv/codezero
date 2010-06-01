/*
 * Copyright B Labs(R) Ltd.
 * Author: Prem Mallappa < prem.mallappa@b-labs.co.uk >
 * Generic memset, still optimized
 */

void 
__attribute__ ((weak))
*memset(void *dst, int c, size_t len)
{
	unsigned char *d=dst;
	c &= 0xff;
	unsigned short cs = (c << 8) | c;
	unsigned long cl = (cs << 16) | cs;
	unsigned align_mask = sizeof(unsigned long) - 1;
	unsigned alignment = (len & align_mask) | 
				((unsigned long)dst & (unsigned long)align_mask);


	switch (alignment) {
		case 3:
			if (len--) {
				*d++ = c;
				alignment--;
			}
		case 2:
			if (len--) {
				*d++ = c;
				alignment--;
			}
		case 1:
			if (len--) {
				*d++ = c;
				alignment--;
			}
		default: break;
	}

	unsigned remain = len & align_mask;
	len -= remain;

	unsigned long *dl = (unsigned long*)d;
	while (len > remain) {
		*dl++ = cl;
		len -= sizeof(unsigned long);
	}
	d = (unsigned char *)dl;

	switch(remain) {
		case 3:
			*d++ = c;
		case 2:
			*d++ = c;
		case 1:
			*d++ = c;
		default: break;
	}

	return dst;
}


#if 0
/* TEST */

int memcmp1(void *dst, int c, size_t len)
{
	char *d = dst;
	int i;
	for(i=0; i < len; i++) {
		printf(" %d: %x \n",i, d[i]);
		if(d[i] != c & 0xff)	
			break;
	}

	if (i < len) {
		printf("diff at %d, dst: %x\n", i, d[i]);
		
		return 1;
	}
	return 0;
}

int main(void)
{
	char d[100];
	int i;
	for (i = 0; i < sizeof(d); i++) {
		d[i] = 0;
	}
		
	memset1(&d[0], 0x55, 4);
	memcmp1(&d[0], 0x55, 4);	

	memset(&d, 0x0, 100);

	memset1(&d[0], 0x55, 2);
	memcmp1(&d[0], 0x55, 2);	
	
	memset(&d, 0x0, 100);

	memset1(&d[0], 0x55, 1);
	memcmp1(&d[0], 0x55, 1);	

	memset(&d, 0x0, 100);

	memset1(&d[1], 0x55, 8);
	memcmp1(&d[1], 0x55, 8);	

	memset(&d, 0x0, 100);

	memset1(&d[1], 0x55, 1);
	memcmp1(&d[1], 0x55, 1);	

	memset(&d, 0x0, 100);

	memset1(&d[1], 0x55, 3);
	memcmp1(&d[1], 0x55, 3);	

	return 0;
}
#endif
