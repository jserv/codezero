/*
 * Mouse details.
 */
#ifndef __MOUSE_H__
#define	__MOUSE_H__

/*
 * Keyboard structure
 */
struct mouse {
	unsigned long base;	/* Virtual base address */
	unsigned long phys_base;  /* Physical address of device */
	int irq_no;	/* IRQ number of device */
};

#endif /* __MOUSE_H__ */
