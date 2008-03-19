#ifndef __ARM_UTCB_H__
#define __ARM_UTCB_H__

#define USER_UTCB_REF           0xFF000FF0
#define L4_KIP_ADDRESS		0xFF000000
#define UTCB_KIP_OFFSET		0xFF0

#ifndef __ASSEMBLY__
#include <l4lib/types.h>
#include <l4/macros.h>
#include INC_GLUE(message.h)

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
#endif /* !__ASSEMBLY__ */

#endif /* __ARM_UTCB_H__ */
