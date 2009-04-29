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

#define EXREGS_SET_PAGER		1
#define	EXREGS_SET_UTCB			2

/* Structure passed by userspace pagers for exchanging registers */
struct exregs_data {
	exregs_context_t context;
	u32 valid_vect;
	u32 flags;
	l4id_t pagerid;
	unsigned long utcb_phys;
	unsigned long utcb_virt;
};



#endif /* __EXREGS_H__ */
