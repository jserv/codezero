#ifndef __LINKER_H__
#define __LINKER_H__
/* 
 *
 * Mock-up copies of variables defined in linker.h
 *
 * Copyright (C) 2005 Bahadir Balban
 *
 */


/* Because no special-case linker.lds is used for tests,
 * actual values for these variables are stored in a linker.c
 */

/* Global that determines where free memory starts.
 * Normally this is the end address of the kernel image
 * in physical memory when it is loaded. See linker.h
 * for other architectures.
 */
extern unsigned int _end;
#endif /* __LINKER_H__ */
