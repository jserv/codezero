#ifndef __ARM_UTCB_H__
#define __ARM_UTCB_H__

#include <l4lib/types.h>
#include <l4lib/arch/vregs.h>

/* FIXME: LICENSE/LICENCE */

/*
 * NOTE: In syslib.h the first few mrs are used by data frequently
 * needed for all ipcs. Those mrs are defined here.
 */

/* MRs always used on receive by syslib */
#define MR_RETURN		0	/* Contains the posix return value. */

/* MRs always used on send by syslib */
#define MR_TAG		0	/* Defines the message purpose */
#define MR_SENDER	1	/* For anythread receivers to discover sender */

/* These define the mr start - end range that aren't used by syslib */
#define MR_UNUSED_START	2	/* The first mr that's not used by syslib.h */
#define MR_TOTAL	6
#define MR_UNUSED_TOTAL		(MR_TOTAL - MR_UNUSED_START)
#define MR_USABLE_TOTAL		MR_UNUSED_TOTAL

 /* Compact utcb for now! :-) */
struct utcb {
	u32 mr[MR_TOTAL];
	u32 tid;		/* Thread id */

	/*
	 * This field is used by servers as the ptr to current tcb,
	 * i.e. the task that this server is serving to.
	 */
	unsigned long usr_handle;
};

static inline struct utcb *__L4_ARM_Utcb()
{
	return (struct utcb *)(*(struct utcb **)USER_UTCB_REF);
}

#endif /* __ARM_UTCB_H__ */
