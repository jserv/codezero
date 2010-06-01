/*
 * Keyboard details.
 */
#ifndef __KEYBOARD_H__
#define	__KEYBOARD_H__

#include <dev/kmi.h>

/*
 * Keyboard structure
 */
struct keyboard {
	unsigned long base;	/* Virtual base address */
	struct keyboard_state state;
	unsigned long phys_base;  /* Physical address of device */
	int irq_no;	/* IRQ number of device */
};

#endif /* __KEYBOARD_H__ */
