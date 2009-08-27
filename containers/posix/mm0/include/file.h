#ifndef __MM0_FILE_H__
#define __MM0_FILE_H__

#include <l4/lib/list.h>
#include <l4lib/types.h>
#include <posix/sys/types.h>
#include <task.h>

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2

int vfs_read(unsigned long vnum, unsigned long f_offset, unsigned long npages,
	     void *pagebuf);
int vfs_write(unsigned long vnum, unsigned long f_offset, unsigned long npages,
	     void *pagebuf);
int sys_read(struct tcb *sender, int fd, void *buf, int count);
int sys_write(struct tcb *sender, int fd, void *buf, int count);
int sys_lseek(struct tcb *sender, int fd, off_t offset, int whence);
int sys_close(struct tcb *sender, int fd);
int sys_fsync(struct tcb *sender, int fd);
int file_open(struct tcb *opener, int fd);

int vfs_open_bypath(const char *pathname, unsigned long *vnum, unsigned long *length);

struct vm_file *do_open2(struct tcb *task, int fd, unsigned long vnum, unsigned long length);
int flush_file_pages(struct vm_file *f);
int read_file_pages(struct vm_file *vmfile, unsigned long pfn_start,
		    unsigned long pfn_end);

struct vfs_file_data {
	unsigned long vnum;
};

#define vm_file_to_vnum(f)	\
	(((struct vfs_file_data *)((f)->priv_data))->vnum)
struct vm_file *vfs_file_create(void);


extern struct link vm_file_list;

#endif /* __MM0_FILE_H__ */
