#ifndef __IPC_H__
#define __IPC_H__

#define L4_NILTHREAD		0xFFFFFFFF
#define L4_ANYTHREAD		0xFFFFFFFE

#define L4_IPC_TAG_MR_OFFSET		0

/* Pagefault */
#define L4_IPC_TAG_PFAULT		0
#define L4_IPC_TAG_UNDEF_FAULT		1

#define L4_IPC_FLAGS_TYPE_MASK		0x0000000F
#define L4_IPC_FLAGS_SHORT		0x00000000	/* Short IPC involves just primary message registers */
#define L4_IPC_FLAGS_FULL		0x00000001	/* Full IPC involves full UTCB copy */
#define L4_IPC_FLAGS_EXTENDED		0x00000002	/* Extended IPC can page-fault and copy up to 2KB */

/* Extended IPC extra fields */
#define L4_IPC_FLAGS_MSG_INDEX_MASK	0x00000FF0	/* Index of message register with buffer pointer */
#define L4_IPC_FLAGS_SIZE_MASK		0x0FFF0000
#define L4_IPC_FLAGS_SIZE_SHIFT		16
#define L4_IPC_FLAGS_MSG_INDEX_SHIFT	4


#define L4_IPC_EXTENDED_MAX_SIZE	(SZ_1K*2)

#endif /* __IPC_H__ */
