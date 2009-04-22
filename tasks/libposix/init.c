/*
 * Main entry point for posix services and applications.
 *
 * Copyright (C) 2007-2009 Bahadir Balban
 */

#include <shpage.h>
#include <posix_init.h>

void posix_init(void)
{
	/* Non-pager tasks initialise their shared communication page */
	if (self_tid() != PAGER_TID)
		shared_page_init();
}

int main(void);

/*
 * Entry point for posix services container.
 *
 * This is executed by all posix system services and tasks
 * that run in this container.
 */
void __container_init(void)
{
	posix_init();
	main();
}

