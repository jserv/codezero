/*
 * IRQ numbers for eb.
 *
 * Copyright (C) 2010 B Labs Ltd.
 *
 */
#ifndef __LIBDEV_EB_IRQ_H__
#define __LIBDEV_EB_IRQ_H__

#if defined (CONFIG_CPU_ARM11MPCORE) || defined (CONFIG_CPU_CORTEXA9)
#define IRQ_TIMER1	34
#define IRQ_KEYBOARD0   39
#define IRQ_MOUSE0	40
#define IRQ_CLCD0	55
#else
#define IRQ_TIMER1	37
#define IRQ_KEYBOARD0	52
#define IRQ_MOUSE0	53
#define IRQ_CLCD0	55
#endif	/* CONFIG_CPU_ARM11MPCORE || CONFIG_CPU_CORTEXA9 */

#endif /* __LIBDEV_EB_IRQ_H__  */
