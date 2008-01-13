#ifndef __TYPES_H__
#define __TYPES_H__

/* Copyright (C) 2003 - 2004 NICTA */

#include <l4lib/arch/types.h>

#define l4_nilpage		0

#define l4_nilthread		-1
#define l4_anythread		-2

#define l4_niltag		0
#define l4_notifytag		(1 << 13)
#define l4_waittag		(1 << 14)

#endif /* __TYPES_H__ */
