/*
 * The disk layout of our simple unix-like filesystem.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban
 */

#ifndef __SFS_LAYOUT_H__
#define __SFS_LAYOUT_H__

#include <l4lib/types.h>

/*
 *
 * Filesystem layout:
 *
 * |---------------|
 * |    Unit 0     |
 * |---------------|
 * |    Unit 1     |
 * |---------------|
 * |      ...      |
 * |---------------|
 * |    Unit n     |
 * |---------------|
 *
 *
 * Unit layout:
 *
 * |---------------|
 * |  Superblock   |
 * |---------------|
 * |  Inode table  |
 * |---------------|
 * |  Data blocks  |
 * |---------------|
 *
 * or
 *
 * |---------------|
 * |  Data blocks  |
 * |---------------|
 *
 */

#define SZ_8MB			(0x100000 * 8)
#define SZ_4KB			0x1000
#define SZ_4KB_BITS		12

#define UNIT_SIZE		(SZ_8MB)
#define INODE_TABLE_SIZE	((UNIT_SIZE >> SZ_4KB_BITS) >> 1)
#define INODE_BITMAP_SZIDX	(INODE_TABLE_SIZE >> 5)

struct sfs_superblock {
	u32 magic;		/* Filesystem magic number */
	u32 fssize;		/* Total size of filesystem */
	u32 szidx;		/* Bitmap index size */
	u32 unitmap[]		/* Bitmap of all fs units */
};

struct sfs_inode_table {
	u32 szidx;
	u32 inodemap[INODE_BITMAP_SZIDX];
	struct sfs_inode inode[INODE_TABLE_SIZE];
};

/*
 * The purpose of an inode:
 *
 * 1) Uniquely identify a file or a directory.
 * 2) Keep file/directory metadata.
 * 3) Provide access means to file blocks/directory contents.
 */

struct sfs_inode_blocks {
	u32  szidx;		/* Direct array index size */
	unsigned long indirect;
	unsigned long indirect2;
	unsigned long indirect3;
	unsigned long direct[];
};

struct sfs_inode {
	u32 unum;	/* Unit number this inode is in */
	u32 inum;	/* Inode number */
	u32 mode;	/* File permissions */
	u32 owner;	/* File owner */
	u32 atime;	/* Last access time */
	u32 mtime;	/* Last content modification */
	u32 ctime;	/* Last inode modification */
	u32 size;	/* Size of contents */
	struct sfs_inode_blocks blocks;
} __attribute__ ((__packed__));

struct sfs_dentry {
	u32 inum;	/* Inode number */
	u32 nlength;	/* Name length */
	u8  name[];	/* Name string */
} __attribute__ ((__packed__));


#endif /* __SFS_LAYOUT_H__ */
