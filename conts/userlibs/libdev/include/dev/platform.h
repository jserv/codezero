/*
 * Generic platform file.
 *
 * Copyright (C) 2010 B Labs Ltd.
 *
 * Author: Bahadir Balban
 */
#ifndef __LIBDEV_PLATFORM_H__
#define __LIBDEV_PLATFORM_H__

#define INC_LIBDEV_PLAT(x)             <dev/platform/__PLATFORM__/x>

/* paths realtive to conts/dev/ */
#include INC_LIBDEV_PLAT(irq.h)
#include INC_LIBDEV_PLAT(platform.h)

#endif /* __LIBDEV_PLATFORM_H__  */
