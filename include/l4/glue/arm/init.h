#ifndef __ARM_GLUE_INIT_H__
#define __ARM_GLUE_INIT_H__

#include <l4/generic/tcb.h>
#include <l4/generic/space.h>

void switch_to_user(struct ktcb *inittask);
void timer_start(void);

extern struct address_space init_space;

#endif /* __ARM_GLUE_INIT_H__ */
