#ifndef __THREAD_H__
#define __THREAD_H__

#define THREAD_CREATE_MASK		0x00F0

/* Create new thread and new space */
#define THREAD_NEW_SPACE		0x0010

/* Create new thread, copy given space */
#define THREAD_COPY_SPACE		0x0020

/* Create new thread, use given space */
#define THREAD_SAME_SPACE		0x0030




#define THREAD_ACTION_MASK		0x000F
#define THREAD_CREATE			0x0000
#define THREAD_RUN			0x0001
#define THREAD_SUSPEND			0x0002
#define THREAD_RESUME			0x0003

#endif /* __THREAD_H__ */
