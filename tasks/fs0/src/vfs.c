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
 * If nothing is found, it reads the vnode from disk into cache. This is called
 * by system calls since tasks keep an fd-to-vnum table.
 */
struct vnode *vfs_lookup_byvnum(struct superblock *sb, unsigned long vnum)
{
	struct vnode *v;
	int err;

	/* Check the vnode cache by vnum */
	list_for_each_entry(v, vnode_cache, cache_list)
		if (v->vnum == vnum)
			return v;

	/*
	 * In the future it will be possible that a vnum known by the fd table
	 * is not in the cache, because the cache will be able to grow and shrink.
	 * But currently it just grows, so its a bug that a known vnum is not in
	 * the cache.
	 */
	BUG();

	/* Check the actual filesystem for the vnode */
	v = vfs_alloc_vnode();
	v->vnum = vnum;
	if ((err = v->read_vnode(sb, v)) < 0) {
		vfs_free_vnode(v);
		return err;
	}

	/* Add the vnode back to vnode cache */
	list_add(&v->cache_list, &vnode_cache);

	return v;
}

/*
 * Vnodes in the vnode cache have 2 keys. One is the set of dentry names they
 * have, the other is their vnum. This one checks the vnode cache by the path
 * first. If nothing is found, it reads the vnode from disk into the cache.
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

