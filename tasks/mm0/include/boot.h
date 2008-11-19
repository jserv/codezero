#ifndef __BOOT_H__
#define __BOOT_H__

#include <vm_area.h>
#include <task.h>

/* Structures to use when sending new task information to vfs */
struct task_data {
	unsigned long tid;
	unsigned long utcb_address;
};

struct task_data_head {
	unsigned long total;
	struct task_data tdata[];
};

int boottask_setup_regions(struct vm_file *file, struct tcb *task,
			   unsigned long task_start, unsigned long task_end);

int boottask_mmap_regions(struct tcb *task, struct vm_file *file);

struct tcb *boottask_exec(struct vm_file *f, unsigned long task_region_start,
			  unsigned long task_region_end, struct task_ids *ids);

int vfs_send_task_data(struct tcb *vfs);

#endif /* __BOOT_H__ */
