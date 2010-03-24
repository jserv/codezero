/*
 * Low-level irq routines.
 *
 * Copyright (C) 2010 B Labs Ltd.
 * Written by  Bahadir Balban
 * Prem Mallappa <prem.mallappa@b-labs.co.uk>
 */

void irq_local_disable_save(unsigned long *state)
{
	unsigned int tmp, tmp2;
	__asm__ __volatile__ (
		"mrs %0, cpsr_fc \n"
		"orr %1, %0, #0x80 \n"
		"msr cpsr_fc, %1 \n"
		: "=&r"(tmp), "=r"(tmp2)
		:
		: "cc"
		);
	*state = tmp;
}

void irq_local_restore(unsigned long state)
{
	__asm__ __volatile__ (
		"msr cpsr_fc, %0\n"
		:
		: "r"(state)
		: "cc"
		);
}

u8 l4_atomic_dest_readb(unsigned long *location)
{
#if 0
	unsigned int tmp;
	__asm__ __volatile__ (
		"swpb	r0, r2, [r1] \n"
		: "=r"(tmp)
		: "r"(location), "r"(0)
		: "memory"
		);

	return (u8)tmp;
#endif

	unsigned int tmp;
	unsigned long state;
	irq_local_disable_save(&state);

	tmp = *location;
	*location = 0;
	
	irq_local_restore(state);

	return (u8)tmp;

}

int irqs_enabled(void)
{
	int tmp;
	__asm__ __volatile__ (
		"mrs %0, cpsr_fc\n"
		: "=r"(tmp)
		);

	if (tmp & 0x80)
		return 0;

	return 1;
}
