/*
 * Generic address space related information.
 *
 * Copyright (C) 2007-2010 Bahadir Balban
 */
#ifndef __SPACE_H__
#define __SPACE_H__

/*
 * Generic mapping flags.
 */
#define MAP_FAULT			0
#define MAP_USR_RW			1
#define MAP_USR_RO			2
#define MAP_KERN_RW			3
#define MAP_USR_IO			4
#define MAP_KERN_IO			5
#define MAP_USR_RWX			6
#define MAP_KERN_RWX			7
#define MAP_USR_RX			8
#define MAP_KERN_RX			9
#define MAP_UNMAP			10	/* For unmap syscall */
#define MAP_INVALID_FLAGS 		(1 << 31)

/* Some default aliases */
#define	MAP_USR_DEFAULT		MAP_USR_RW
#define MAP_KERN_DEFAULT	MAP_KERN_RW
#define MAP_IO_DEFAULT		MAP_KERN_IO

#endif /* __SPACE_H__ */
