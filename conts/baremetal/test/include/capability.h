#ifndef __CAPABILITY_H__
#define __CAPABILITY_H__

#include <l4/api/capability.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>

void capability_print(struct capability *cap);

int caps_read_all();

#endif /* __CAPABILITY_H__ */
