#ifndef __MM0_PROC__
#define __MM0_PROC__

#include <vm_area.h>

struct proc_files {
	struct vm_object *stack_file;	/* ZI, RO: devzero, RW: private */
	struct vm_object *env_file;	/* NON-ZI, RO: private, RW: private */
	struct vm_object *data_file;	/* NON-ZI, RO: shared,  RW: private */
	struct vm_object *bss_file;	/* ZI, RO: devzero, RW: private */
};

int task_setup_vm_objects(struct tcb *t);

#endif
