/*
 *
 */
#include <stdio.h>

extern void utcb_test(unsigned long utcb_start, unsigned long utcb_end);

void l4thread_print(void)
{
	printf("Hello world from libl4thread!\n");
}
