/*
 * An imaginary filesystem for reading the in-memory
 * server tasks loaded out of the initial elf executable.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <fs.h>
#include <l4/lib/list.h>

/*
 * These are preallocated structures for forging a filesystem tree
 * from the elf loaded server segments available from kdata info.
 */
#define BOOTFS_IMG_MAX					10
struct vnode bootfs_vnode[BOOTFS_IMG_MAX];
struct dentry bootfs_dentry[BOOTFS_IMG_MAX];
struct file bootfs_file[BOOTFS_IMG_MAX];

/* These are for the root */
struct dentry bootfs_root;
struct vnode bootfs_rootvn;

void bootfs_init_root(struct dentry *r)
{
	struct vnode *v = r->vnode;

	/* Initialise dentry for rootdir */
	r->refcnt = 0;
	strcpy(r->name, "");
	INIT_LIST_HEAD(&r->child);
	INIT_LIST_HEAD(&r->children);
	INIT_LIST_HEAD(&r->vref);
	r->parent = r;

	/* Initialise vnode for rootdir */
	v->id = 0;
	v->refcnt = 0;
	INIT_LIST_HEAD(&v->dirents);
	INIT_LIST_HEAD(&v->state_list);
	list_add(&r->vref, &v->dirents);
	v->size = 0;
}

struct superblock *bootfs_init_sb(struct superblock *sb)
{
	sb->root = &bootfs_root;
	sb->root->vnode = &bootfs_rootvn;

	bootfs_init_root(&sb->root);

	return sb;
}

struct superblock bootfs_sb = {
	.fs = bootfs_type,
	.ops = {
		.read_sb = bootfs_read_sb,
		.write_sb = bootfs_write_sb,
		.read_vnode = bootfs_read_vnode,
		.write_vnode = bootfs_write_vnode,
	},
};

struct superblock *bootfs_get_sb(void)
{
	bootfs_init_sb(&bootfs_sb);
	return &bootfs_sb;
}

struct file_system_type bootfs_type = {
	.name = "bootfs",
	.magic = 0,
	.get_sb = bootfs_get_sb,
};

void init_bootfs()
{
	struct superblock *sb;

	sb = bootfs_type.get_sb();
}

