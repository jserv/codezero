
#ifndef __CAP_PRINT_H__
#define __CAP_PRINT_H__

#include <l4/api/capability.h>
#include <l4/generic/cap-types.h>

void cap_dev_print(struct capability *cap);
void cap_print(struct capability *cap);
void cap_array_print(int total_caps, struct capability *caparray);

#endif /* __CAP_PRINT_H__*/
