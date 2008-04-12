/*
 * FS0 Initialisation.
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <fs.h>
#include <vfs.h>
#include <bdev.h>
#include <task.h>
#include <kdata.h>
#include <stdio.h>
#include <string.h>
#include <l4/lib/list.h>
#include <l4/api/errno.h>
#include <memfs/memfs.h>

struct list_head fs_type_list;

struct superblock *vfs_probe_filesystems(void *block)
{
	struct file_system_type *fstype;
	struct superblock *sb;

	list_for_each_entry(fstype, &fs_type_list, list) {
		/* Does the superblock match for this fs type? */
		if ((sb = fstype->ops.get_superblock(block))) {
			/*
			 * Add this to the list of superblocks this
			 * fs already has.
			 */
			list_add(&sb->list, &fstype->sblist);
			return sb;
		}
	}

	return PTR_ERR(-ENODEV);
}

/*
 * Registers each available filesystem so that these can be
 * used when probing superblocks on block devices.
 */
void vfs_register_filesystems(void)
{
	/* Initialise fstype list */
	INIT_LIST_HEAD(&fs_type_list);

	/* Call per-fs registration functions */
	memfs_register_fstype(&fs_type_list);
}

/*
 * Filesystem initialisation.
 */
int initialise(void)
{
	void *rootdev_blocks;
	struct superblock *root_sb;

	/* Get standard init data from microkernel */
	request_initdata(&initdata);

	/* Register compiled-in filesystems with vfs core. */
	vfs_register_filesystems();

	/* Get a pointer to first block of root block device */
	rootdev_blocks = vfs_rootdev_open();

	/*
	 * Since the *only* filesystem we have is a temporary memory
	 * filesystem, we create it on the root device first.
	 */
	memfs_format_filesystem(rootdev_blocks);

	/* Search for a filesystem on the root device */
	BUG_ON(IS_ERR(root_sb = vfs_probe_filesystems(rootdev_blocks)));

	/* Mount the filesystem on the root device */
	vfs_mount_root(root_sb);

	/* Learn about what tasks are running */
	init_task_data();

	/*
	 * Initialisation is done. From here on, we can start
	 * serving filesystem requests.
	 */
	return 0;
}

