/*
 * UTCB handling helper routines
 *
 * Copyright (C) 2009 B Labs Ltd.
 */
#ifndef __LIB_UTCB_H__
#define __LIB_UTCB_H__

#include "utcb-common.h"

#define UTCB_START_ADDR(utcb)	((unsigned long)(utcb))
#define UTCB_END_ADDR(utcb) \
			((unsigned long)((utcb) + (sizeof(utcb))))

int set_utcb_params(unsigned long utcb_start_addr,
			unsigned long utcb_end_addr);

//#define MAPPING_ENABLE

#if defined(MAPPING_ENABLE)

#define IS_UTCB_SETUP()		(utcb_table_ptr)

struct utcb_entry *utcb_table_ptr;

struct utcb_entry {
	struct utcb_desc *udesc;
	unsigned long utcb_phys_base;
};

unsigned long get_utcb_addr(struct task_ids *parent_id, struct task_ids *child_id);
#else /* !MAPPING_ENABLE */

#define IS_UTCB_SETUP()		(udesc_ptr)

struct utcb_desc *udesc_ptr;

unsigned long get_utcb_addr(void);
#endif /* MAPPING_ENABLE */

#endif /* __LIB_UTCB_H__ */
