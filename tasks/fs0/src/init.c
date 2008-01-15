/*
 * FS0 Initialisation.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <kdata.h>
#include <fs.h>
#include <string.h>
#include <stdio.h>
#include <l4/lib/list.h>
#include <init.h>

struct filesystem bootfs = {
	.magic = 0,
	.name = "Tempfs for boot images",
};

struct superblock bootfs_sb;

#define BOOTFS_IMG_MAX					10
struct vnode bootfs_vnode[BOOTFS_IMG_MAX];
struct dentry bootfs_dentry[BOOTFS_IMG_MAX];
struct file bootfs_file[BOOTFS_IMG_MAX];

struct rootdir {
	struct dentry *d;
	struct filesystem *fs;
};
struct rootdir bootfs_root;

void init_root(struct vnode *root_vn, struct dentry *root_d)
{
	/* Initialise dentry for rootdir */
	root_d->refcnt = 0;
	strcpy(root_d->name, "");
	INIT_LIST_HEAD(&root_d->child);
	INIT_LIST_HEAD(&root_d->children);
	INIT_LIST_HEAD(&root_d->dref_list);
	root_d->vnode = root_vn;

	/* Initialise vnode for rootdir */
	root_vn->id = 0;
	root_vn->refcnt = 0;
	INIT_LIST_HEAD(&root_vn->dirents);
	INIT_LIST_HEAD(&root_vn->state_list);
	list_add(&root_d->dref_list, &root_vn->dirents);
	root_vn->size = 0;

	/* Initialise global struct rootdir ptr */
	bootfs_root.d = root_d;
	bootfs_root.fs = &bootfs;
}

#define	PATH_SEP		"/"
#define PATH_CURDIR		"."
#define PATH_OUTDIR		".."

void fs_debug_list_all(struct dentry *root)
{
	struct dentry *d;
	int stotal = 0;

	/* List paths first */
	printf("%s%s\n", root->name, PATH_SEP);
	list_for_each_entry(d, &root->children, child) {
		printf("%s%s%s, size: 0x%x\n", d->parent->name, PATH_SEP, d->name, d->vnode->size);
		stotal += d->vnode->size;
	}
}

void init_bootfs(struct initdata *initdata)
{
	struct bootdesc *bd = initdata->bootdesc;
	struct dentry *img_d = &bootfs_dentry[1];
	struct vnode *img_vn = &bootfs_vnode[1];
	struct file *img_f = &bootfs_file[1];
	struct svc_image *img;

	/* The first vfs object slot is for the root */
	init_root(&bootfs_vnode[0], &bootfs_dentry[0]);

	BUG_ON(bd->total_images >= BOOTFS_IMG_MAX);
	for (int i = 0; i < bd->total_images; i++) {
		img = &bd->images[i];

		/* Initialise dentry for image */
		img_d->refcnt = 0;
		strncpy(img_d->name, img->name, VFS_DENTRY_NAME_MAX);
		INIT_LIST_HEAD(&img_d->child);
		INIT_LIST_HEAD(&img_d->children);
		img_d->vnode = img_vn;
		img_d->parent = bootfs_root.d;
		list_add(&img_d->child, &bootfs_root.d->children);

		/* Initialise vnode for image */
		img_vn->id = img->phys_start;
		img_vn->refcnt = 0;
		INIT_LIST_HEAD(&img_vn->dirents);
		list_add(&img_d->dref_list, &img_vn->dirents);
		img_vn->size = img->phys_end - img->phys_start;

		/* Initialise file struct for image */
		img_f->refcnt = 0;
		img_f->dentry = img_d;

		img_d++;
		img_vn++;
		img_f++;
	}
}

void initialise(void)
{
	request_initdata(&initdata);
	init_bootfs(&initdata);

	/* A debug call that lists all vfs structures */
	fs_debug_list_all(bootfs_root.d);
}

