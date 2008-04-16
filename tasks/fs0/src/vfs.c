/*
 * High-level vfs implementation.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <fs.h>
#include <vfs.h>
#include <task.h>
#include <path.h>

LIST_HEAD(vnode_cache);
LIST_HEAD(dentry_cache);

/*
 * /
 * /boot
 * /boot/images/mm0.axf
 * /boot/images/fs0.axf
 * /boot/images/test0.axf
 * /file.txt
 */
struct vfs_mountpoint vfs_root;

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

	/* Check the vnode flat list by vnum */
	list_for_each_entry(v, &vnode_cache, cache_list)
		if (v->vnum == vnum)
			return v;

	/* Check the actual filesystem for the vnode */
	v = vfs_alloc_vnode();
	v->vnum = vnum;

	/* Note this only checks given superblock */
	if ((err = sb->ops->read_vnode(sb, v)) < 0) {
		vfs_free_vnode(v);
		return PTR_ERR(err);
	}

	/* Add the vnode back to vnode flat list */
	list_add(&v->cache_list, &vnode_cache);

	return v;
}

/*
 * Vnodes in the vnode cache have 2 keys. One is the set of dentry names they
 * have, the other is their vnum. This one checks the vnode cache by the path
 * first. If nothing is found, it reads the vnode from disk into the cache.
 */
struct vnode *vfs_lookup_bypath(struct pathdata *pdata)
{
	const char *firstcomp;

	/*
	 * This does vfs cache + fs lookup.
	 */
	BUG_ON(list_empty(&pdata->list));
       	firstcomp = pathdata_next_component(pdata);
	return pdata->vstart->ops.lookup(pdata->vstart, pdata, firstcomp);
}

int vfs_mount_root(struct superblock *sb)
{
	/*
	 * Lookup the root vnode of this superblock.
	 * The root superblock has vnode number 0.
	 */
	vfs_root.pivot = vfs_lookup_byvnum(sb, 0);
	vfs_root.sb = sb;

	return 0;
}

