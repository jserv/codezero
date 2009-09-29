/*
 * Container entry point for this task
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */
#include <posix/posix_init.h>
#include <l4lib/init.h>
#include <l4lib/utcb.h>

void main(void);

void __container_init(void)
{
	/* Generic L4 thread initialisation */
	__l4_init();

	/* FS0 posix-service initialisation */
	posix_service_init();

	/* Entry to main */
	main();
}

