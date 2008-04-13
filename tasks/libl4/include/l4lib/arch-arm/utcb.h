#ifndef __ARM_UTCB_H__
#define __ARM_UTCB_H__

#define USER_UTCB_REF           0xFF000FF0
#define L4_KIP_ADDRESS		0xFF000000
#define UTCB_KIP_OFFSET		0xFF0

#ifndef __ASSEMBLY__
#include <l4lib/types.h>
#include <l4/macros.h>
#include INC_GLUE(message.h)
#include INC_GLUE(memory.h)
#include <string.h>
#include <stdio.h>

/*
 * NOTE: In syslib.h the first few mrs are used by data frequently
 * needed for all ipcs. Those mrs are defined the kernel message.h
 */

/*
 * This is a per-task private structure where message registers are
 * pushed for ipc. Its *not* TLS, but can be part of TLS when it is
 * supported.
 */
struct utcb {
	u32 mr[MR_TOTAL];
	u32 tid;		/* Thread id */
} __attribute__((__packed__));

extern struct utcb utcb;
extern void *utcb_page;

static inline struct utcb *l4_get_utcb()
{
	return &utcb;
}

/* Functions to read/write utcb registers */
static inline unsigned int read_mr(int offset)
{
	return l4_get_utcb()->mr[offset];
}

static inline void write_mr(unsigned int offset, unsigned int val)
{
	l4_get_utcb()->mr[offset] = val;
}

/*
 * Arguments that are too large to fit in message registers are
 * copied onto another area that is still on the utcb, and the servers
 * map-in the task utcb and read those arguments from there.
 */

static inline void copy_to_utcb(void *arg, int offset, int size)
{
	BUG_ON(size > PAGE_SIZE);
	memcpy(utcb_page, arg, size);
}

static inline void copy_from_utcb(void *buf, int offset, int size)
{
	BUG_ON(size > PAGE_SIZE);
	memcpy(buf, utcb_page + offset, size);
}

#endif /* !__ASSEMBLY__ */

#endif /* __ARM_UTCB_H__ */
