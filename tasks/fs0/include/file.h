#ifndef __FS0_MM_H__
#define __FS0_MM_H__

/*
 * Describes the in-memory representation of a file. This is used to track
 * file content, i.e. file pages by mm0, this is a temporary mock up until
 * fs0 and mm0 are wired together.
 */
struct vm_file {
	unsigned long vnum;
	unsigned long length;

	/* This is the cache of physical pages that this file has in memory. */
	struct list_head page_cache_list;
	struct vm_pager *pager;
};

#endif /* __FS0_MM_H__ */
