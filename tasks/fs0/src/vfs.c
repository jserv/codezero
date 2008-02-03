/*
 * High-level vfs implementation.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <fs.h>
#include <vfs.h>

struct vnode *vfs_lookup(struct superblock *sb, char *path)
{
	/* If it's just / we already got it. */
	if (!strcmp(path, "/"))
		return sb->root;

	return sb->root->ops.lookup(sb->root, path);
}

/*
 * /
 * /boot
 * /boot/images/mm0.axf
 * /boot/images/fs0.axf
 * /boot/images/test0.axf
 * /file.txt
 */
struct vfs_mountpoint vfs_root;

int vfs_mount_root(struct superblock *sb)
{
	/* Lookup the root vnode of this superblock */
	vfs_root.pivot = vfs_lookup(sb, "/");
	vfs_root.sb = sb;

	return 0;
}

