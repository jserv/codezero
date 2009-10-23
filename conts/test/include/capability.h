#ifndef __CAPABILITY_H__
#define __CAPABILITY_H__

#include <l4/api/capability.h>
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>

void print_capability(struct capability *cap);

int read_pager_capabilities();



#endif /* __CAPABILITY_H__ */
