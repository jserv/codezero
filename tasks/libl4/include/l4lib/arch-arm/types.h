#ifndef __L4_ARCH_ARM__
#define __L4_ARCH_ARM__

#define TASK_ID_INVALID			-1
struct task_ids {
	int tid;
	int spid;
	int tgid;
};

#include <l4/arch/arm/types.h>

#endif
