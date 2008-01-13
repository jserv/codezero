/*
 * Generic irq handling definitions.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#ifndef __GENERIC_IRQ_H__
#define __GENERIC_IRQ_H__

#include <l4/lib/string.h>
#include INC_PLAT(irq.h)

/* Represents none or spurious irq */
#define IRQ_NIL					(-1)

/* Successful irq handling state */
#define IRQ_HANDLED				0

typedef void (*irq_op_t)(int irq);
struct irq_chip_ops {
	void (*init)(void);
	int (*read_irq)(void);
	irq_op_t ack_and_mask;
	irq_op_t unmask;
};

struct irq_chip {
	char name[32];
	int level;		/* Cascading level */
	int cascade;		/* The irq that lower chip uses on this chip */
	int offset;		/* The global offset for this irq chip */
	struct irq_chip_ops ops;
};

typedef int (*irq_handler_t)(void);
struct irq_desc {
	char name[8];
	struct irq_chip *chip;

	/* TODO: This could be a list for multiple handlers */
	irq_handler_t handler;
};

extern struct irq_desc irq_desc_array[];
extern struct irq_chip irq_chip_array[];

static inline void irq_enable(int irq_index)
{
	struct irq_desc *this_irq = irq_desc_array + irq_index;
	struct irq_chip *this_chip = this_irq->chip;

	this_chip->ops.unmask(irq_index - this_chip->offset);
}

static inline void irq_disable(int irq_index)
{
	struct irq_desc *this_irq = irq_desc_array + irq_index;
	struct irq_chip *this_chip = this_irq->chip;

	this_chip->ops.ack_and_mask(irq_index - this_chip->offset);
}

static inline void register_irq(char *name, int irq_index, irq_handler_t handler)
{
	struct irq_desc *this_desc = irq_desc_array + irq_index;
	struct irq_chip *current_chip = irq_chip_array;

	strncpy(&this_desc->name[0], name, sizeof(this_desc->name));

	for (int i = 0; i < IRQ_CHIPS_MAX; i++)
		if (irq_index <= current_chip->offset) {
			this_desc->chip = current_chip;
			break;
		}
	this_desc->handler = handler;
}

void do_irq(void);
void irq_controllers_init(void);

#endif /* __GENERIC_IRQ_H__ */
