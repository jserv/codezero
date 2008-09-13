/*
 * Generic to arch-specific interface for
 * exchange_registers()
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <exregs.h>


void exregs_set_stack(struct exregs_data *s, unsigned long sp)
{
	s->context.sp = sp;
	s->valid_vect |= 1 << (offsetof(task_context_t, sp) >> 2);
}

void exregs_set_pc(struct exregs_data *s, unsigned long pc)
{
	s->context.pc = pc;
	s->valid_vect |= 1 << (offsetof(task_context_t, pc) >> 2);
}

