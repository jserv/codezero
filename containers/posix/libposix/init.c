/*
 * Main entry point for posix services and applications.
 *
 * Copyright (C) 2007-2009 Bahadir Balban
 */
#include <shpage.h>
#include <posix_init.h>
#include <libposix.h>

void posix_service_init(void)
{
	/* Non-pager tasks initialise their shared communication page */
	BUG_ON(self_tid() != VFS_TID);
	shared_page_init();
}

void libposix_init(void)
{
	/* Shall only be run by posix applications */
	BUG_ON(self_tid() == PAGER_TID || self_tid() == VFS_TID);
	shared_page_init();
}

