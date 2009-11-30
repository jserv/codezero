#ifndef __UART_SERVICE_CAPABILITY_H__
#define __UART_SERVICE_CAPABILITY_H__

#include <l4lib/capability.h>
#include <l4/api/capability.h>
#include <l4/generic/cap-types.h>

void cap_print(struct capability *cap);
void cap_list_print(struct cap_list *cap_list);
int cap_read_all();

#endif /* header */
