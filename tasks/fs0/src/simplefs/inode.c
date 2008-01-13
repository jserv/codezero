/*
 * A basic unix-like read/writeable filesystem.
 *
 * Copyright (C) 2007 Bahadir Balban
 */

#define SIMPLEFS_BLOCK_SIZE		1024
/*
 * Simplefs superblock
 */
struct simplefs_sb {
	unsigned long magic;	/* Filesystem magic */
	unsigned long ioffset;	/* Offset of first inode */
	unsigned long bsize;	/* Fs block size */
};

struct simplefs_sb *sb;

static void simplefs_fill_super(struct superblock *sb)
{
	char buf[SIMPLEFS_BLOCK_SIZE];

	bdev_read(0, SIMPLEFS_BLOCK_SIZE, buf);

}

static void simplefs_read_vnode(struct vnode *)
{
}


