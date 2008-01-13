#ifndef __ARM_MESSAGE_H__
#define __ARM_MESSAGE_H__

#include <l4lib/types.h>
#include <l4lib/arch/utcb.h>

/* Functions to read/write utcb registers */
static inline unsigned int read_mr(int offset)
{
	return __L4_ARM_Utcb()->mr[offset];
}

static inline void write_mr(unsigned int val, unsigned int offset)
{
	__L4_ARM_Utcb()->mr[offset] = val;
}

/* Tag definitions */
#define L4_TAG_PFLAG	(1 << 12)	/* Propagation */
#define L4_TAG_NFLAG	(1 << 13)	/* Notify */
#define L4_TAG_RFLAG	(1 << 14)	/* Block-on-receive */
#define L4_TAG_SFLAG	(1 << 15)	/* Block-on-send */
#define L4_TAG_XFLAG	(1 << 14)	/* Crosscall (inter-cpu ipc) */
#define L4_TAG_EFLAG	(1 << 15)	/* Error */

#endif /* __ARM_MESSAGE_H__ */
