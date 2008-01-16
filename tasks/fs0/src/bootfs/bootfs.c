/*
 * A pseudo-filesystem for reading the in-memory
 * server tasks loaded from the initial elf executable.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */
#include <fs.h>
#include <l4/lib/list.h>
#include <malloc.h>

struct dentry *bootfs_dentry_lookup(struct dentry *d, char *dname)
{
	struct dentry *this;

	list_for_each_entry(this, child, &d->children) {
		if (this->compare(this, dname))
			return this;
	}
	return 0;
}

struct dentry *path_lookup(struct superblock *sb, char *pathstr)
{
	char *dname;
	char *splitpath;
	struct dentry *this, *next;

	/* First dentry is root */
	this = sb->root;

	/* Get next path component from path string */
	dname = path_next_dentry_name(pathstr);

	if (!this->compare(dname))
		return;

	while(!(dname = path_next_dentry_name(pathstr))) {
		if ((d = this->lookup(this, dname)))
			return 0;
	}
}

/*
 * This creates a pseudo-filesystem tree from the loaded
 * server elf images whose information is available from
 * initdata.
 */
void bootfs_populate(struct initdata *initdata, struct superblock *sb)
{
	struct bootdesc *bd = initdata->bootdesc;
	struct svc_image *img;
	struct dentry *d;
	struct vnode *v;
	struct file *f;

	for (int i = 0; i < bd->total_images; i++) {
		img = &bd->images[i];

		d = malloc(sizeof(struct dentry));
		v = malloc(sizeof(struct vnode));
		f = malloc(sizeof(struct file));

		/* Initialise dentry for image */
		d->refcnt = 0;
		d->vnode = v;
		d->parent = sb->root;
		strncpy(d->name, img->name, VFS_DENTRY_NAME_MAX);
		INIT_LIST_HEAD(&d->child);
		INIT_LIST_HEAD(&d->children);
		list_add(&d->child, &sb->root->children);

		/* Initialise vnode for image */
		v->refcnt = 0;
		v->id = img->phys_start;
		v->size = img->phys_end - img->phys_start;
		INIT_LIST_HEAD(&v->dirents);
		list_add(&d->v_ref, &v->dirents);

		/* Initialise file struct for image */
		f->refcnt = 0;
		f->dentry = d;

		img_d++;
		img_vn++;
		img_f++;
	}
}

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
	sb->root = malloc(sizeof(struct dentry));
	sb->root->vnode = malloc(sizeof(struct vnode));

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
	bootfs_init_sb(&bootfs_sb);

	bootfs_populate(&bootfs_sb);
}
