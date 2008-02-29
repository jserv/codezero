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

/* Compact utcb for now! :-) */
struct utcb {
	u32 mr[MR_TOTAL];
	u32 tid;		/* Thread id */

	/*
	 * For passing ipc data larger than mrs,
	 * that is, if the callee is allowed to map it
	 */
	char buf[];
};
extern struct utcb *utcb;

static inline struct utcb *l4_get_utcb()
{
	return utcb;
	// (struct utcb **)USER_UTCB_REF;
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
