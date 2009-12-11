#ifndef __L4LIB_ARCH_IRQ_H__
#define __L4LIB_ARCH_IRQ_H__


/*
 * Destructive atomic-read.
 *
 * Write 0 to byte at @location as its contents are read back.
 */
static inline char l4_atomic_dest_readb(void *location)
{
	unsigned char zero = 0;
	unsigned char val;
	char *loc = location;

	__asm__ __volatile__ (
		"swpb	%1, %2, [%0] \n"
		:: "r" (*loc), "r" (val), "r" (zero)
	);

	return val;
}


#endif
