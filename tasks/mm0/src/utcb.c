/*
 * Management of task utcb regions and own utcb.
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */

#include <l4lib/arch/utcb.h>
#include <l4/macros.h>

/*
 * UTCB management in Codezero:
 *
 * 1.) Every task in the system defines an array of utcbs in a special .utcb
 *     section that is page-aligned and to be kept wired in by the pager.
 * 2.) The region marks are written to bootdesc structure at compile-time.
 * 3.) Pager reads the bootdesc struct from the microkernel during init.
 * 4.) Pager initialises structures to alloc/dealloc new utcbs for every address
 *     space.
 * 5.) Pagers dynamically allocate utcb addresses as each thread is created
 *     via a thread_control() system call.
 * 6.) Each thread in an address space learns their utcb address from a
 *     well-defined KIP offset. This is updated as each thread becomes runnable.
 */




