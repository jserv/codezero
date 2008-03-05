#ifndef __MM0_PROC__
#define __MM0_PROC__

#include <vm_area.h>

struct proc_vm_objects {
	struct vm_object *stack;	/* ZI, RO: devzero, RW: private */
	struct vm_object *env;		/* NON-ZI, RO: private, RW: private */
	struct vm_object *data;		/* NON-ZI, RO: shared,  RW: private */
	struct vm_object *bss;		/* ZI, RO: devzero, RW: private */
};

int task_setup_vm_objects(struct tcb *t);

#endif
