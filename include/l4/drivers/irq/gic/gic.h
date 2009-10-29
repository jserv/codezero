/*
 * Generic Interrupt Controller offsets
 *
 * Copyright (C) 2007 Bahadir Balban
 *
 */

#ifndef __ARM_GIC_H__
#define __ARM_GIC_H__

#include INC_PLAT(platform.h)

/* GIC CPU register offsets */
#define ARM_GIC_CPU_ICR		0x00 /* Interface Control */
#define ARM_GIC_CPUPMR		0x04 /* Interrupt Priority Mask */
#define ARM_GIC_CPU_BPR		0x08 /* Binary Point */
#define ARM_GIC_CPU_IAR		0x0c /* Interrupt Acknowledge */
#define ARM_GIC_CPU_EOIR	0x10 /* End of Interrupt */
#define ARM_GIC_CPU_RRI		0x14 /* Running Priority */
#define ARM_GIC_CPU_HPIR	0x18 /* Highest Priority Interrupt*/

/* Distributor register map */
#define ARM_GIC_DIST_CR		0x000 /* Control Register */
#define ARM_GIC_DIST_ICTR	0x004 /* Interface Controller Type */
#define ARM_GIC_DIST_ISER	0x100 /* Interrupt Set Enable */
#define ARM_GIC_DIST_ICER	0x180 /* Interrupt Clear Enable */
#define ARM_GIC_DIST_ISPR	0x200 /* Interrupt Set Pending */
#define ARM_GIC_DIST_ICPR	0x280 /* Interrupt Clear Pending*/
#define ARM_GIC_DIST_ABR	0x300 /* Active Bit */
#define ARM_GIC_DIST_IPR	0x400 /* Interrupt Priority */
#define ARM_GIC_DIST_IPTR	0x800 /* Interrupt Processor Target */
#define ARM_GIC_DIST_ICR	0xc00 /* Interrupt Configuration */
#define ARM_GIC_DIST_SGIR	0xf00 /* Software Generated Interrupt */

#endif /* __ARM_GIC_H__ */
