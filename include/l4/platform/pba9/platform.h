#ifndef __PBA9_PLATFORM_H__
#define __PBA9_PLATFORM_H__
/*
 * Platform specific ties between drivers and generic APIs used by the kernel.
 * E.g. system timer and console.
 *
 * Copyright (C) Bahadir Balban 2007
 */

#include <l4/platform/realview/platform.h>

#define CLCD_SELECT_BOARD	0
#define CLCD_SELECT_TILE	1

void sri_set_mux(int offset);
void clcd_clock_write(u32 freq, int clcd_select);
u32 clcd_clock_read(int clcd_select);
void clcd_init(void);
void kmi_init(void);

#endif /* __PBA9_PLATFORM_H__ */
