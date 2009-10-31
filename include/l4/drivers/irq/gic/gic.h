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
#define ARM_GIC_CPU_IC		0x00 /* Interface Control */
#define ARM_GIC_CPUPM		0x04 /* Interrupt Priority Mask */
#define ARM_GIC_CPU_BP		0x08 /* Binary Point */
#define ARM_GIC_CPU_IA		0x0c /* Interrupt Acknowledge */
#define ARM_GIC_CPU_EOI		0x10 /* End of Interrupt */
#define ARM_GIC_CPU_RPI		0x14 /* Running Priority */
#define ARM_GIC_CPU_HPI		0x18 /* Highest Priority Interrupt*/

/* Distributor register map */
#define ARM_GIC_DIST_CNTRL	0x000 /* Control Register */
#define ARM_GIC_DIST_ICT	0x004 /* Interface Controller Type */
#define ARM_GIC_DIST_ISE	0x100 /* Interrupt Set Enable */
#define ARM_GIC_DIST_ICE	0x180 /* Interrupt Clear Enable */
#define ARM_GIC_DIST_ISP	0x200 /* Interrupt Set Pending */
#define ARM_GIC_DIST_ICP	0x280 /* Interrupt Clear Pending*/
#define ARM_GIC_DIST_AB		0x300 /* Active Bit */
#define ARM_GIC_DIST_IP		0x400 /* Interrupt Priority */
#define ARM_GIC_DIST_IPT	0x800 /* Interrupt Processor Target */
#define ARM_GIC_DIST_IC		0xc00 /* Interrupt Configuration */
#define ARM_GIC_DIST_SGI	0xf00 /* Software Generated Interrupt */

#endif /* __ARM_GIC_H__ */
