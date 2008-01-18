/*
 * VFS definitions.
 *
 * Copyright (C) 2007 Bahadir Balban.
 */
#ifndef __FS_H__
#define __FS_H__
#include <l4/lib/list.h>

typedef void (*dentry_op_t)(void);
typedef void (*superblock_op_t)(void);
typedef void (*vnode_op_t)(void);
typedef void (*file_op_t)(void);

struct dentry_ops {
	dentry_op_t compare;
};

struct file_ops {
	file_op_t open;
	file_op_t read;
	file_op_t write;
	file_op_t close;
	file_op_t mmap;
	file_op_t lseek;
	file_op_t flush;
	file_op_t fsync;
};

struct vnode_ops {
	vnode_op_t create;
	vnode_op_t lookup;
	vnode_op_t link;
	vnode_op_t unlink;
	vnode_op_t mkdir;
	vnode_op_t rmdir;
	vnode_op_t rename;
	vnode_op_t getattr;
	vnode_op_t setattr;
};

struct superblock_ops {
	superblock_op_t read_sb;
	superblock_op_t write_sb;
	superblock_op_t read_vnode;
	superblock_op_t write_vnode;
};

struct dentry;
struct file;
struct file_system_type;
struct superblock;
struct vnode;

#define	VFS_DENTRY_NAME_MAX		512
struct dentry {
	int refcnt;
	char name[VFS_DENTRY_NAME_MAX];
	struct dentry *parent;		/* Parent dentry */
	struct list_head child;		/* List of dentries with same parent */
	struct list_head children;	/* List of children dentries */
	struct list_head vref;		/* For vnode's dirent reference list */
	struct vnode *vnode;		/* The vnode associated with dirent */
	struct dentry_ops ops;
};

struct file {
	int refcnt;
	struct dentry *dentry;
	struct file_ops ops;
};

struct vnode {
	unsigned long id;		/* Filesystem-wide unique vnode id */
	int refcnt;			/* Reference counter */
	int hardlinks;			/* Number of hard links */
	struct vnode_ops ops;		/* Operations on this vnode */
	struct list_head dirents;	/* Dirents that refer to this vnode */
	struct list_head state_list;	/* List for vnode's dirty/clean state */
	unsigned long size;		/* Total size of vnode in bytes */
};

struct file_system_type {
	char name[256];
	unsigned long magic;
	unsigned int flags;
	struct superblock *(*get_sb)(void);
	struct list_head sb_list;
};

struct superblock {
	struct file_system_type fs;
	struct superblock_ops ops;
	struct dentry *root_dirent;
};


#endif /* __FS_H__ */
