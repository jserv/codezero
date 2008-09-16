#ifndef __MM0_FILE_H__
#define __MM0_FILE_H__

#include <l4/lib/list.h>
#include <l4lib/types.h>
#include <posix/sys/types.h>
#include <task.h>

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
int sys_close(l4id_t sender, int fd);
int sys_fsync(l4id_t sender, int fd);
int file_open(struct tcb *opener, int fd);

struct vfs_file_data {
	unsigned long vnum;
};

#define vm_file_to_vnum(f)	\
	(((struct vfs_file_data *)((f)->priv_data))->vnum)
struct vm_file *vfs_file_create(void);


extern struct list_head vm_file_list;

#endif /* __MM0_FILE_H__ */
