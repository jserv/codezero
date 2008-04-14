/*
 * Copyright (C) 2008 Bahadir Balban
 *
 * Path lookup related information
 */
#ifndef __PATH_H__
#define __PATH_H__

/*
 * FIXME:
 * These ought to be strings and split/comparison functions should
 * always use strings because formats like UTF-8 wouldn't work.
 */
#define VFS_CHAR_SEP		'/'
#define VFS_STR_ROOTDIR		"/"
#define VFS_STR_PARDIR		".."
#define VFS_STR_CURDIR		"."
#define VFS_STR_XATDIR		"...."

struct pathdata {
	struct tcb *task;
	int root;
	char *path;
};

#endif /* __PATH_H__ */
