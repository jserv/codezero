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
	file_op_t seek;
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
struct filesystem;
struct superblock;
struct vnode;

struct dentry {
	int refcnt;
	char name[512];
	struct dentry *parent;		/* Parent dentry */
	struct list_head siblings;	/* List of dentries with same parent */
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
	struct vnode_ops ops;		/* Operations on this vnode */
	struct list_head dirents;	/* Dirents that refer to this vnode */
	struct list_head state_list;	/* List for vnode's dirty/clean state */
	unsigned long size;		/* Total size of vnode in bytes */
};

struct filesystem {
	unsigned long magic;
	char name[256];
};

struct superblock {
	struct filesystem fs;
	struct superblock_ops ops;
	struct dentry *root_dirent;
};

#endif /* __FS_H__ */
