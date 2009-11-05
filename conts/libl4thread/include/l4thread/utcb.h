/*
 * UTCB handling helper routines
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_UTCB_H__
#define __LIB_UTCB_H__

#define IS_UTCB_SETUP()		(udesc_ptr)

struct utcb_desc *udesc_ptr;

unsigned long get_utcb_addr(void);

#endif /* __LIB_UTCB_H__ */
