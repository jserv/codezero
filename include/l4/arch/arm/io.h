#ifndef __ARM_IO_H__
#define __ARM_IO_H__
/*
 * Arch-specific io functions/macros.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#define read(val, address)	val = *((volatile unsigned int *) address)
#define write(val, address)	*((volatile unsigned int *) address) = val

#endif /* __ARM_IO_H__ */
