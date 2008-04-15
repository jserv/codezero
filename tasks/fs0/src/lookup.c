/*
 * Inode lookup.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <fs.h>
#include <vfs.h>
#include <stat.h>
#include <l4/api/errno.h>
#include <lib/pathstr.h>

/*
 * Given a dentry that has been populated by readdir with children dentries
 * and their vnodes, this itself checks all those children on that level for
 * a match, but it also calls lookup, which recursively checks lower levels.
 */
struct vnode *lookup_dentry_children(struct dentry *parentdir,
				     struct pathdata *pdata)
{
	struct dentry *childdir;
	struct vnode *v;

	list_for_each_entry(childdir, &parentdir->children, child)
		/* Non-zero result means either found it or error occured */
		if ((v = childdir->vnode->ops.lookup(childdir->vnode, pdata)))
			return v;

	/* Out of all children dentries, neither was a match, so we return 0 */
	return PTR_ERR(-ENOENT);
}

/*
 * Check if we're really at root, return root string if so, and
 * move the pdata string cursor to next component or the end of string.
 */
char *pathdata_handle_root(struct pathdata *pdata)
{
	char *cursor = pdata->path;

	/* Only handle if root */
	if (!pdata->root)
		return 0;

	/* Move forward until no seperator */
	while (*cursor == VFS_CHAR_SEP) {
		*cursor = '\0';	/* Zero out separators */
		cursor++;	/* Move to next */
	}

	/*
	 * Moved the cursor either to first char of
	 * next component, or the end of string
	 * (in which case cursor = '\0')
	 */
	pdata->path = cursor;

	/* We have handled the root case, now turn it off */
	pdata->root = 0;

	/*
	 * Modified pdata appropriately, return the root
	 * component. It's the root representer string, e.g. "/"
	 */
	return VFS_STR_ROOTDIR;
}

/* Lookup, recursive, assuming single-mountpoint */
struct vnode *generic_vnode_lookup(struct vnode *thisnode,
				   struct pathdata *pdata)
{
	char *component;
	struct dentry *d;
	struct vnode *found;
	int err;

	/*
	 * Determine first component, taking root as special case.
	 *
	 * path (root = 0) -> first path component
	 * path (root = 1) -> VFS_STR_ROOTDIR
	 * where path = { "//comp1/comp2", "/", "/comp1/comp2", "comp1/comp2" }
	 */

	printf("Looking up: %s\n", pdata->path);

	/* Handle the special root case */
	if (pdata->root)
		component =  pathdata_handle_root(pdata);
	/* Handle paths normally */
	else
		component = splitpath(&pdata->path, VFS_CHAR_SEP);

	/* Does this path component match with any of this vnode's dentries? */
	list_for_each_entry(d, &thisnode->dentries, vref) {
		if (d->ops.compare(d, component)) {
			/* Is this a directory? */
			if (vfs_isdir(thisnode)) {
				/* Are there any more path components? */
				if (*pdata->path) {
					/* Read directory contents */
					if ((err = d->vnode->ops.readdir(d->vnode)) < 0)
						return PTR_ERR(err);

					/* Search all children one level below. */
					if ((found = lookup_dentry_children(d, pdata)))
					 	/* Either found, or non-zero error */
						return found;
				} else
					return thisnode;
			} else { /* Its a file */
				if (*pdata->path) /* There's still path, but not directory */
					return PTR_ERR(-ENOTDIR);
				else /* No path left, found it, so return file */
					return thisnode;
			}
		}
	}

	/* Not found, return nothing */
	return PTR_ERR(-ENOENT);
}

int generic_dentry_compare(struct dentry *d, char *name)
{
	if (!strcmp(d->name, name) || !strcmp(name, VFS_STR_CURDIR))
		return 1;
	else
		return 0;
}

struct dentry_ops generic_dentry_operations = {
	.compare = generic_dentry_compare,
};

