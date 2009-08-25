#ifndef __INITTASK_ARCH_MM_H__
#define __INITTASK_ARCH_MM_H__

#include <arch/offsets.h>
#include <l4/macros.h>
#include <l4/types.h>
#include INC_GLUE(memory.h)
#include <vm_area.h>

#define INITTASK_ADDR(x)	((x >= INITTASK_AREA_START) && (x < INITTASK_AREA_END))

struct fault_data;
unsigned int vm_prot_flags(pte_t pte);
void set_generic_fault_params(struct fault_data *fault);

#endif /* __INITTASK_ARCH_MM_H__ */
