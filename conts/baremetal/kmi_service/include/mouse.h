/*
 * Mouse details.
 */
#ifndef __MOUSE_H__
#define	__MOUSE_H__

#include <l4lib/mutex.h>
#include <l4/lib/list.h>
#include <l4lib/types.h>

/*
 * Keyboard structure
 */
struct mouse {
	unsigned long base;	/* Virtual base address */
	struct capability cap;  /* Capability describing keyboard */
};

#endif /* __MOUSE_H__ */
