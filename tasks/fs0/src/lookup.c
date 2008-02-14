/*
 * Inode lookup.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <fs.h>
#include <vfs.h>
#include <stat.h>
#include <l4/api/errno.h>

/*
 * Splits the string str according to the given seperator, returns the
 * first component, and modifies the str so that it points at the next
 * available component (or a leading separator which can be filtered
 * out on a subsequent call to this function).
 */
char *splitpath(char **str, char sep)
{
	char *cursor = *str;
	char *end;

	/* Move forward until no seperator */
	while (*cursor == sep) {
		*cursor = '\0';
		cursor++;	/* Move to first char of component */
	}

	end = cursor;
	while (*end != sep && *end != '\0')
		end++;		/* Move until end of component */

	if (*end == sep) {	/* if ended with separator */
		*end = '\0';	/* finish component by null */
		if (*(end + 1) != '\0')	/* if more components after, */
			*str = end + 1;	/* assign beginning to str */
		else
			*str = end; /* else str is also depleted, give null */
	} else /* if end was null, that means the end for str, too. */
		*str = end;

	return cursor;
}

/*
 * Given a dentry that has been populated by readdir with children dentries
 * and their vnodes, this itself checks all those children on that level for
 * a match, but it also calls lookup, which recursively checks lower levels.
 */
struct vnode *lookup_dentry_children(struct dentry *parentdir, char *path)
{
	struct dentry *childdir;
	struct vnode *v;

	list_for_each_entry(childdir, &parentdir->children, child)
		/* Non-zero result means either found it or error occured */
		if ((v = childdir->vnode->ops.lookup(childdir->vnode, path)))
			return v;

	/* Out of all children dentries, neither was a match, so we return 0 */
	return 0;
}

/* Lookup, recursive, assuming single-mountpoint */
struct vnode *generic_vnode_lookup(struct vnode *thisnode, char *path)
{
	char *component = splitpath(&path, VFS_STR_SEP);
	struct dentry *d;
	struct vnode *found;
	int err;

	/* Does this path component match with any of parent vnode's names? */
	list_for_each_entry(d, &thisnode->dentries, vref) {
		if (d->ops.compare(d, component)) {
			/* Is this a directory? */
			if (vfs_isdir(thisnode)) {
				/* Are there any more path components? */
				if (path) {
					/* Read directory contents */
					if ((err = d->vnode->ops.readdir(d->vnode)) < 0)
						return PTR_ERR(err);

					/* Search all children one level below. */
					if ((found = lookup_dentry_children(d, path)))
					 	/* Either found, or non-zero error */
						return found;
				} else
					return thisnode;
			} else { /* Its a file */
				if (path) /* There's still path, but not directory */
					return PTR_ERR(-ENOTDIR);
				else /* No path left, found it, so return file */
					return thisnode;
			}
		}
	}

	/* Not found, return nothing */
	return 0;
}

int generic_dentry_compare(struct dentry *d, char *name)
{
	if (!strcmp(d->name, name))
		return 1;
	else
		return 0;
}

struct dentry_ops generic_dentry_operations = {
	.compare = generic_dentry_compare,
};

