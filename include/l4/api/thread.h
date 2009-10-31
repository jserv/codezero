#ifndef __API_THREAD_H__
#define __API_THREAD_H__

#define THREAD_ACTION_MASK	0x000F
#define THREAD_CREATE		0x0000
#define THREAD_RUN		0x0001
#define THREAD_SUSPEND		0x0002
#define THREAD_DESTROY		0x0003
#define THREAD_RECYCLE		0x0004

#define THREAD_CREATE_MASK	0x0FF0
#define TC_SHARE_CAPS		0x0010	/* Share all thread capabilities */
#define TC_SHARE_UTCB		0x0020	/* Share utcb location (same space */
#define TC_SHARE_GROUP		0x0040	/* Share thread group id */
#define TC_SHARE_SPACE		0x0080	/* New thread, use given space */
#define TC_COPY_SPACE		0x0100	/* New thread, copy given space */
#define TC_NEW_SPACE		0x0200	/* New thread, new space */
#define TC_SHARE_PAGER		0x0400  /* New thread, shared pager */
#define TC_AS_PAGER		0x0800	/* Set new thread as child */

#endif /* __API_THREAD_H__ */
