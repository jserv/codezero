/*
 * High-level vfs implementation.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <fs.h>
#include <vfs.h>

struct list_head vnode_cache;
struct list_head dentry_cache;

/*
 * Vnodes in the vnode cache have 2 keys. One is their dentry names, the other
 * is their vnum. This one checks the vnode cache by the given vnum first.
 * If nothing is found, it reads the vnode from disk into cache.
 */
struct vnode *vfs_lookup_byvnum(struct superblock *sb, unsigned long vnum)
{
	struct vnode *v;

	/* Check the vnode cache by vnum */
	list_for_each_entry(v, vnode_cache, cache_list)
		if (v->vnum == vnum)
			return v;

	/* Otherwise ask the filesystem:
	 * TODO:
	 * vfs_alloc_vnode() -> This should also add the empty vnode to vnode cache.
	 * read_vnode() -> read it from filesystem.
	 * return what's read.
	 */
}

/*
 * Vnodes in the vnode cache have 2 keys. One is their dentry name, the
 * other is their vnum. This one checks the vnode cache by the path first.
 * If nothing is found, it reads the vnode from disk into the cache.
 */
struct vnode *vfs_lookup_bypath(struct superblock *sb, char *path)
{
	/* If it's just / we already got it. */
	if (!strcmp(path, "/"))
		return sb->root;

	/* TODO: Check the vnode cache by path component
	 *
	 * This should split the path into components and compare
	 * each of them with each vnode's dentry list.
	 */

	/* It's not in the cache, look it up from filesystem. */
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
	vfs_root.pivot = vfs_lookup_bypath(sb, "/");
	vfs_root.sb = sb;

	return 0;
}

