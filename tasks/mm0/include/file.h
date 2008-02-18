#ifndef __MM0_FILE_H__
#define __MM0_FILE_H__

#include <l4/lib/list.h>
#include <posix/sys/types.h>

void vmfile_init(void);
struct vm_file *vmfile_alloc_init(void);
int vfs_receive_sys_open(l4id_t sender, l4id_t opener, int fd,
			 unsigned long vnum, unsigned long size);
int vfs_read(unsigned long vnum, unsigned long f_offset, unsigned long npages,
	     void *pagebuf);
int vfs_write(unsigned long vnum, unsigned long f_offset, unsigned long npages,
	     void *pagebuf);
int sys_read(l4id_t sender, int fd, void *buf, int count);
int sys_write(l4id_t sender, int fd, void *buf, int count);
int sys_lseek(l4id_t sender, int fd, off_t offset, int whence);

#endif /* __MM0_FILE_H__ */
