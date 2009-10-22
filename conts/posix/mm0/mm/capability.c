/*
 * Pager's capabilities for kernel resources
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#include <bootm.h>
#include <init.h>
#include <l4/lib/list.h>
#include <capability.h>
#include <l4/api/capability.h>
#include <l4lib/arch/syscalls.h>
#include <l4/generic/cap-types.h>	/* TODO: Move this to API */
#include <malloc/malloc.h>

