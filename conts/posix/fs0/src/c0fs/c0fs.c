/*
 * A basic unix-like read/writeable filesystem for Codezero.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <init.h>
#include <fs.h>

void sfs_read_sb(struct superblock *sb)
{

}

static void simplefs_alloc_vnode(struct vnode *)
{

}

struct file_system_type sfs_type = {
	.name = "c0fs",
	.magic = 1,
	.flags = 0,
};

/* Registers sfs as an available filesystem type */
void sfs_register_fstype(struct link *fslist)
{
	list_insert(&sfs_type.list, fslist);
}
