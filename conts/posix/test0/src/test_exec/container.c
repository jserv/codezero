/*
 * Container entry point for this task.
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */

#include <l4lib/types.h>
#include <l4lib/init.h>
#include <l4lib/utcb.h>
#include <posix_init.h>		/* Initialisers for posix library */

void main(void);

void __container_init(void)
{
	/* Generic L4 thread initialisation */
	__l4_init();

	/* Entry to main */
	main();
}

