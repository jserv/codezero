#ifndef __THREAD_H__
#define __THREAD_H__

#define THREAD_CREATE_MASK		0x0030

/* Create new thread and new space */
#define THREAD_NEW_SPACE		0x0010

/* Create new thread, copy given space */
#define THREAD_COPY_SPACE		0x0020

/* Create new thread, use given space */
#define THREAD_SAME_SPACE		0x0030

/* Shared UTCB, New UTCB, No UTCB */
#define THREAD_UTCB_MASK		0x00C0
#define THREAD_UTCB_NEW			0x0040
#define THREAD_UTCB_SAME		0x0080
#define THREAD_UTCB_NONE		0x00C0


#define THREAD_ACTION_MASK		0x000F
#define THREAD_CREATE			0x0000
#define THREAD_RUN			0x0001
#define THREAD_SUSPEND			0x0002
#define THREAD_RESUME			0x0003
#define THREAD_DESTROY			0x0004
#define THREAD_RECYCLE			0x0005

#endif /* __THREAD_H__ */
