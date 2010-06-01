/*
 * ARMv7 specific abort handling definitions
 *
 * Copyright (C) 2010 B Labs Ltd.
 */
#ifndef __V7_ARCH_EXCEPTION_H__
#define __V7_ARCH_EXCEPTION_H__

/* Data and Prefetch abort encodings */
#define ABORT_TTBW_SYNC_EXTERNAL_LEVEL1		0x0C
#define ABORT_TTBW_SYNC_EXTERNAL_LEVEL2 	0x0E
#define ABORT_TTBW_SYNC_PARITY_LEVEL1		0x1C
#define ABORT_TTBW_SYNC_PARITY_LEVEL2		0x1E
#define ABORT_TRANSLATION_SECTION		0x05
#define ABORT_TRANSLATION_PAGE			0x07
#define ABORT_AFLAG_SECTION			0x03
#define ABORT_AFLAG_PAGE			0x06
#define ABORT_DOMAIN_SECTION			0x09
#define ABORT_DOMAIN_PAGE			0x0B
#define ABORT_PERMISSION_SECTION		0x0D
#define ABORT_PERMISSION_PAGE			0x0F
#define ABORT_DEBUG_EVENT			0x02
#define ABORT_SYNC_EXTERNAL			0x08
#define ABORT_SYNC_PARITY			0x19
#define ABORT_ASYNC_PARITY			0x18 /* Only on Data aborts */
#define ABORT_ASYNC_EXTERNAL			0x16 /* Only on Data aborts */
#define ABORT_ICACHE_MAINTENANCE		0x04 /* Only in Data aborts */
#define ABORT_ALIGNMENT				0x01 /* Only in Data aborts */

/* IFSR/DFSR register bits */
#define FSR_FS_BIT4				10  /* 4th bit of fault status */
#define DFSR_WNR_BIT				11  /* Write-not-read bit */
#define FSR_EXT_BIT				12  /* External abort type bit */
#define FSR_FS_MASK				0xF

static inline u32 fsr_get_status(u32 fsr)
{
	return (fsr & FSR_FS_MASK) |
	       (((fsr >> FSR_FS_BIT4) & 1) << 4);
}

#endif /* __V7_ARCH_EXCEPTION_H__ */
