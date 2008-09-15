/*
 * Exchange registers system call data.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#ifndef __EXREGS_H__
#define __EXREGS_H__

#include <l4/macros.h>
#include INC_GLUE(syscall.h)
#include INC_GLUE(context.h)
#include <l4/types.h>

enum exregs_flags {
	EXREGS_SET_PAGER = 1,
};

/* Structure passed by userspace pagers for exchanging registers */
struct exregs_data {
	exregs_context_t context;
	u32 valid_vect;
	u32 flags;
	l4id_t pagerid;
};



#endif /* __EXREGS_H__ */
