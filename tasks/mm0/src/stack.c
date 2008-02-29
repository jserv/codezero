/*
 * TODO: This is to be used when moving mm0's temporary
 * stack to a proper place. It's unused for now.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <l4/config.h>
#include <l4/macros.h>
#include <l4/types.h>
#include INC_GLUE(memlayout.h)
#include <string.h>

/* The initial temporary stack used until memory is set up */
__attribute__ ((section("init.stack"))) char stack[4096];
extern unsigned long __stack[];		/* Linker defined */

/* Moves from temporary stack to where it should be in actual. */
void move_stack()
{
	register unsigned int sp asm("sp");
	register unsigned int fp asm("r11");

	unsigned int stack_offset = (unsigned long)__stack - sp;
	unsigned int frame_offset = (unsigned long)__stack - fp;

	/* Copy current stack into new stack. NOTE: This might demand-page
	 * the new stack, but maybe that won't work. */
	memcpy((void *)USER_AREA_END, __stack, stack_offset);
	sp = USER_AREA_END - stack_offset;
	fp = USER_AREA_END - frame_offset;

}

