#ifndef __THREAD_H__
#define __THREAD_H__

#define THREAD_FLAGS_MASK		0x00F0

/* Create new thread, copy given space */
#define THREAD_CREATE_COPYSPC		0x0010

/* Create new thread and new space */
#define THREAD_CREATE_NEWSPC		0x0020

/* Create new thread, use given space */
#define THREAD_CREATE_SAMESPC		0x0030


#define THREAD_ACTION_MASK		0x000F
#define THREAD_CREATE			0x0000
#define THREAD_RUN			0x0001
#define THREAD_SUSPEND			0x0002
#define THREAD_RESUME			0x0003

#endif /* __THREAD_H__ */
