/*
 * Definitions for executables
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#ifndef __EXEC_H__
#define __EXEC_H__

/*
 * This presents extra executable file information that is
 * not present in the tcb, in a generic format.
 */ 
struct exec_file_desc {
	unsigned long text_offset;	/* File offset of text section */
	unsigned long data_offset;	/* File offset of data section */
};

#endif /* __EXEC_H__ */
