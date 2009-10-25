/*
 * Types of capabilities and their operations
 *
 * Copyright (C) 2009 Bahadir Balban
 */
#ifndef __CAP_TYPES_H__
#define __CAP_TYPES_H__

/*
 * Capability types
 */
#define CAP_TYPE_MASK		0x0000FFFF
#define CAP_TYPE_TCTRL		(1 << 0)
#define CAP_TYPE_EXREGS		(1 << 1)
#define CAP_TYPE_MAP		(1 << 2)
#define CAP_TYPE_IPC		(1 << 3)
#define CAP_TYPE_SCHED		(1 << 4)
#define CAP_TYPE_UMUTEX		(1 << 5)
#define CAP_TYPE_QUANTITY	(1 << 6)
#define CAP_TYPE_CAP		(1 << 7)
#define cap_type(c)	((c)->type & CAP_TYPE_MASK)

/*
 * Resource types
 */
#define CAP_RTYPE_MASK		0xFFFF0000
#define CAP_RTYPE_THREAD	(1 << 16)
#define CAP_RTYPE_TGROUP	(1 << 17)
#define CAP_RTYPE_SPACE		(1 << 18)
#define CAP_RTYPE_CONTAINER	(1 << 19)
#define CAP_RTYPE_UMUTEX	(1 << 20) /* Don't mix with pool version */
#define CAP_RTYPE_VIRTMEM	(1 << 21)
#define CAP_RTYPE_PHYSMEM	(1 << 22)
#define CAP_RTYPE_CPUPOOL	(1 << 23)
#define CAP_RTYPE_THREADPOOL	(1 << 24)
#define CAP_RTYPE_SPACEPOOL	(1 << 25)
#define CAP_RTYPE_MUTEXPOOL	(1 << 26)
#define CAP_RTYPE_MAPPOOL	(1 << 27) /* For pmd spending */
#define CAP_RTYPE_CAPPOOL	(1 << 28) /* For new cap generation */
#define CAP_RTYPE_PGGROUP	(1 << 29) /* Group of paged threads */

#define cap_rtype(c)	((c)->type & CAP_RTYPE_MASK)

/*
 * Access permissions
 */

/* Thread control capability */
#define CAP_TCTRL_CREATE	(1 << 0)
#define CAP_TCTRL_DESTROY	(1 << 1)
#define CAP_TCTRL_SUSPEND	(1 << 2)
#define CAP_TCTRL_RESUME	(1 << 3)
#define CAP_TCTRL_RECYCLE	(1 << 4)

/* Exchange registers capability */
#define CAP_EXREGS_RW_PAGER	(1 << 0)
#define CAP_EXREGS_RW_UTCB	(1 << 1)
#define CAP_EXREGS_RW_SP	(1 << 2)
#define CAP_EXREGS_RW_PC	(1 << 3)
#define CAP_EXREGS_RW_REGS	(1 << 4) /* Other regular regs */
#define CAP_EXREGS_RW_CPU	(1 << 5)
#define CAP_EXREGS_RW_CPUTIME	(1 << 6)

/* Map capability */

/* Shift values */
#define CAP_MAP_READ_BIT	0
#define CAP_MAP_WRITE_BIT	1
#define CAP_MAP_EXEC_BIT	2
#define CAP_MAP_CACHED_BIT	3
#define CAP_MAP_UNCACHED_BIT	4
#define CAP_MAP_UNMAP_BIT	5
#define CAP_MAP_UTCB_BIT	6

#define CAP_MAP_READ		(1 << CAP_MAP_READ_BIT)
#define CAP_MAP_WRITE		(1 << CAP_MAP_WRITE_BIT)
#define CAP_MAP_EXEC		(1 << CAP_MAP_EXEC_BIT)
#define CAP_MAP_CACHED		(1 << CAP_MAP_CACHED_BIT)
#define CAP_MAP_UNCACHED	(1 << CAP_MAP_UNCACHED_BIT)
#define CAP_MAP_UNMAP		(1 << CAP_MAP_UNMAP_BIT)
#define CAP_MAP_UTCB		(1 << CAP_MAP_UTCB_BIT)

/* Ipc capability */
#define CAP_IPC_SEND		(1 << 0)
#define CAP_IPC_RECV		(1 << 1)
#define CAP_IPC_SHORT		(1 << 2)
#define CAP_IPC_FULL		(1 << 3)
#define CAP_IPC_EXTENDED	(1 << 4)
#define CAP_IPC_ASYNC		(1 << 5)

/* Userspace mutex capability */
#define CAP_UMUTEX_LOCK		(1 << 0)
#define CAP_UMUTEX_UNLOCK	(1 << 1)

/* Capability control capability */
#define CAP_CAP_MODIFY		(1 << 0)
#define CAP_CAP_GRANT		(1 << 1)
#define CAP_CAP_READ		(1 << 2)
#define CAP_CAP_SHARE		(1 << 3)

#endif /* __CAP_TYPES_H__ */
