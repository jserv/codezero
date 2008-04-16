/*
 * Path manipulation functions.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <l4/macros.h>
#include <l4/lib/list.h>
#include <l4/api/errno.h>
#include <lib/pathstr.h>
#include <lib/malloc.h>
#include <path.h>
#include <stdio.h>
#include <fs.h>
#include <task.h>

const char *pathdata_next_component(struct pathdata *pdata)
{
	struct pathcomp *p, *n;
	const char *pathstr;

	list_for_each_entry_safe(p, n, &pdata->list, list) {
		list_del(&p->list);
		pathstr = p->str;
		kfree(p);
		return pathstr;
	}
	return "";
}

/* Check there's at least one element, unlink and return the last element */
const char *pathdata_last_component(struct pathdata *pdata)
{
	struct pathcomp *p;
	const char *pathstr;

	if (!list_empty(&pdata->list)) {
		p = list_entry(pdata->list.prev, struct pathcomp, list);
		list_del(&p->list);
		pathstr = p->str;
		kfree(p);
		return pathstr;
	}

	return "";
}

/* Unlink and free all path components in pathdata, and then free pathdata */
void pathdata_destroy(struct pathdata *p)
{
	struct pathcomp *c, *n;

	list_for_each_entry_safe(c, n, &p->list, list) {
		list_del(&c->list);
		kfree(c);
	}
	kfree(p);
}

void pathdata_print(struct pathdata *p)
{
	struct pathcomp *comp;

	printf("Extracted path is:\n");
	list_for_each_entry(comp, &p->list, list)
		printf("%s\n", comp->str);
}

/* Extracts all path components from pathname into more presentable form */
struct pathdata *pathdata_parse(const char *pathname,
				char *pathbuf, struct tcb *task)
{
	struct pathdata *pdata = kzalloc(sizeof(*pdata));
	struct pathcomp *comp;
	char *str;

	if (!pdata)
		return PTR_ERR(-ENOMEM);

	/* Initialise pathdata */
	INIT_LIST_HEAD(&pdata->list);
	strcpy(pathbuf, pathname);

	/* First component is root if there's a root */
	if (pathname[0] == VFS_CHAR_SEP) {
		if (!(comp = kzalloc(sizeof(*comp)))) {
			kfree(pdata);
			return PTR_ERR(-ENOMEM);
		}
		INIT_LIST_HEAD(&comp->list);
		comp->str = VFS_STR_ROOTDIR;
		list_add_tail(&comp->list, &pdata->list);

		/* Lookup start vnode is root vnode */
		pdata->vstart = task->rootdir;

	/* Otherwise start from current directory */
	} else {
		struct dentry *curdir;

		if (!(comp = kzalloc(sizeof(*comp)))) {
			kfree(pdata);
			return PTR_ERR(-ENOMEM);
		}
		INIT_LIST_HEAD(&comp->list);

		/* Get current dentry for this task */
		curdir = list_entry(task->curdir->dentries.next,
				    struct dentry, vref);

		/* Use its name in path component */
		comp->str = curdir->name;
		list_add_tail(&comp->list, &pdata->list);

		/* Lookup start vnode is current dir vnode */
		pdata->vstart = task->curdir;
	}

	/* Add every other path component */
	str = splitpath(&pathbuf, VFS_CHAR_SEP);
	while(*str) {
		/* Any curdir components in path are ignored. */
		if (!strcmp(str, VFS_STR_CURDIR)) {
			;
		} else {
			if (!(comp = kzalloc(sizeof(*comp)))) {
				pathdata_destroy(pdata);
				return PTR_ERR(-ENOMEM);
			}
			INIT_LIST_HEAD(&comp->list);
			comp->str = str;
			list_add_tail(&comp->list, &pdata->list);
		}

		/* Next component */
		str = splitpath(&pathbuf, VFS_CHAR_SEP);
	}
	pathdata_print(pdata);

	return pdata;
}

