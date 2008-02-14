/*
 * Inode and vnode implementation.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <fs.h>
#include <vfs.h>
#include <memfs/memfs.h>
#include <memfs/file.h>
#include <l4/lib/list.h>
#include <l4/api/errno.h>
#include <l4/macros.h>
#include <lib/malloc.h>
#include <stdio.h>


struct memfs_inode *memfs_alloc_inode(struct memfs_superblock *sb)
{
	struct mem_cache *cache;
	struct memfs_inode *i;
	void *free_block;

	/* Ask existing inode caches for a new inode */
	list_for_each_entry(cache, &sb->inode_cache_list, list) {
		if (cache->free)
			if (!(i =  mem_cache_zalloc(cache)))
				return PTR_ERR(-ENOSPC);
			else
				return i;
		else
			continue;
	}

	/* Ask existing block caches for a new block */
	if (IS_ERR(free_block = memfs_alloc_block(sb)))
		return PTR_ERR(free_block);

	/* Initialise it as an inode cache */
	cache = mem_cache_init(free_block, sb->blocksize,
			       sizeof(struct memfs_inode), 0);
	list_add(&cache->list, &sb->inode_cache_list);

	if (!(i =  mem_cache_zalloc(cache)))
		return PTR_ERR(-ENOSPC);
	else
		return i;
}

/*
 * O(n^2) complexity but its simple, yet it would only reveal on high numbers.
 */
int memfs_free_inode(struct memfs_superblock *sb, struct memfs_inode *i)
{
	struct mem_cache *c, *tmp;

	list_for_each_entry_safe(c, tmp, &sb->inode_cache_list, list) {
		/* Free it, if success */
		if (!mem_cache_free(c, i)) {
			/* If cache completely emtpy */
			if (mem_cache_is_empty(c)) {
				/* Free the block, too. */
				list_del(&c->list);
				memfs_free_block(sb, c);
			}
			return 0;
		}
	}
	return -EINVAL;
}

/* Allocates *and* initialises the inode */
struct memfs_inode *memfs_create_inode(struct memfs_superblock *sb)
{
	struct memfs_inode *i;

	/* Allocate the inode */
	if (PTR_ERR(i = memfs_alloc_inode(sb)) < 0)
		return i;

	/* Allocate a new inode number */
	if ((i->inum = id_new(sb->ipool)) < 0)
		return i;

	/* Put a reference to this inode in the inode table at this index */
	sb->inode[i->inum] = i;

	return i;
}

/* Deallocate the inode and any other closely relevant structure */
int memfs_destroy_inode(struct memfs_superblock *sb, struct memfs_inode *i)
{
	int inum = i->inum;

	/* Deallocate the inode */
	if (memfs_free_inode(sb, i) < 0)
		return -EINVAL;

	/* Deallocate the inode number */
	if (id_del(sb->ipool, inum) < 0)
		return -EINVAL;

	/* Clear the ref in inode table */
	sb->inode[inum] = 0;

	return 0;
}

struct vnode *memfs_alloc_vnode(struct superblock *sb)
{
	struct memfs_inode *i;
	struct vnode *v;

	/* Get a (pseudo-disk) memfs inode */
	if (IS_ERR(i = memfs_create_inode(sb->fs_super)))
		return PTR_ERR(i);

	/* Get a vnode */
	if (!(v = vfs_alloc_vnode()))
		return PTR_ERR(-ENOMEM);

	/* Associate the two together */
	v->inode = i;
	v->vnum = i->inum;

	/* Associate memfs-specific fields with vnode */
	v->ops = memfs_vnode_operations;
	v->fops = memfs_file_operations;

	/* Return the vnode */
	return v;
}

int memfs_free_vnode(struct superblock *sb, struct vnode *v)
{
	struct memfs_inode *i = v->inode;

	BUG_ON(!i); /* Vnodes that come here must have valid inodes */

	/* Destroy on-disk inode */
	memfs_destroy_inode(sb->fs_super, i);

	/* Free vnode */
	vfs_free_vnode(v);

	return 0;
}

/*
 * Given a vnode with a valid vnum, this retrieves the corresponding
 * inode from the filesystem.
 */
struct memfs_inode *memfs_read_inode(struct superblock *sb, struct vnode *v)
{
	struct memfs_superblock *fssb = sb->fs_super;

	BUG_ON(!fssb->inode[v->vnum]);

	return fssb->inode[v->vnum];
}

/*
 * Given a preallocated vnode with a valid vnum, this reads the corresponding
 * inode from the filesystem and fills in the vnode's fields.
 */
int memfs_read_vnode(struct superblock *sb, struct vnode *v)
{
	struct memfs_inode *i = memfs_read_inode(sb, v);

	if (!i)
		return -EEXIST;

	/* Simply copy common fields */
	v->vnum = i->inum;
	v->size = i->size;
	v->mode = i->mode;
	v->owner = i->owner;
	v->atime = i->atime;
	v->mtime = i->mtime;
	v->ctime = i->ctime;

	return 0;
}

int memfs_write_vnode(struct superblock *sb, struct vnode *v)
{
	struct memfs_inode *i = v->inode;

	/* Vnodes that come here must have valid inodes */
	BUG_ON(!i);

	/* Simply copy common fields */
	i->inum = v->vnum;
	i->size = v->size;
	i->mode = v->mode;
	i->owner = v->owner;
	i->atime = v->atime;
	i->mtime = v->mtime;
	i->ctime = v->ctime;

	return 0;
}


/*
 * Given a non-zero dirbuf, uses it, otherwise it allocates one on its own.
 *
 * Allocates and populates all dentries and their corresponding vnodes that are
 * the direct children of vnode v. This means that by each call to readdir, the vfs
 * layer increases its cache of filesystem tree by one level beneath that directory.
 *
 * TODO: Returns the list of posix-compliant dirent records in the buffer.
 */
int memfs_vnode_readdir(struct vnode *v)
{
	int err;
	u8 *dirbuf;
	struct memfs_dentry *memfsd;
	struct dentry *parent = list_entry(v->dentries.next,
					   struct dentry, vref);

	/* Check directory type */
	if (!vfs_isdir(v))
		return -ENOTDIR;

	/* Allocate dirbuf if one is not provided by the upper layer */
	if (!v->dirbuf->buffer) {
		/* This is as big as a page */
		v->dirbuf->buffer = vfs_alloc_dirbuf();
		memfsd = dirbuf = v->dirbuf->buffer;

		/*
		 * Fail if vnode size is bigger than a page. Since this allocation
		 * method is to be replaced, we can live with this limitation for now.
		 */
		BUG_ON(v->size > PAGE_SIZE);
	}

	/* Read memfsd contents into the buffer */
	if ((err = v->fops.read(v, 0, 1, dirbuf)))
		return err;

	/* Read fs-specific directory entry into vnode and dentry caches. */
	for (int i = 0; i < (v->size / sizeof(struct memfs_dentry)); i++) {
		struct dentry *newd;
		struct vnode *newv;

		/* Allocate a vfs dentry */
		if (!(newd = vfs_alloc_dentry()))
			return -ENOMEM;

		/* Initialise it */
		newd->ops = generic_dentry_operations;
		newd->parent = parent;
		list_add(&newd->child, &parent->children);

		/*
		 * Read the vnode for dentry by its vnode number.
		 */
		newv = newd->vnode = vfs_alloc_vnode();
		newv->vnum = memfsd[i].inum;
		BUG_ON(newv->sb->ops->read_vnode(newv->sb, newv) < 0);

		/* Assing this dentry as a name of its vnode */
		list_add(&newd->vref, &newd->vnode->dentries);

		/* Copy fields into generic dentry */
		memcpy(newd->name, memfsd[i].name, MEMFS_DNAME_MAX);

		/* Add both vnode and dentry to their caches */
		list_add(&newd->cache_list, &dentry_cache);
		list_add(&newv->cache_list, &vnode_cache);
	}
	return 0;
}

struct vnode_ops memfs_vnode_operations = {
	.lookup = generic_vnode_lookup,
	.readdir = memfs_vnode_readdir,
};

struct superblock_ops memfs_superblock_operations = {
	.read_vnode = memfs_read_vnode,
	.write_vnode = memfs_write_vnode,
	.alloc_vnode = memfs_alloc_vnode,
	.free_vnode = memfs_free_vnode,
};

