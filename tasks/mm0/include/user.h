#ifndef __USER_H__
#define __USER_H__

#include <task.h>

int pager_validate_user_range(struct tcb *user, void *userptr, unsigned long size,
			      unsigned int vm_flags);
void *pager_validate_map_user_range(struct tcb *user, void *userptr,
				    unsigned long size, unsigned int vm_flags);
void pager_unmap_user_range(void *mapped_ptr, unsigned long size);

#endif /* __USER_H__ */
