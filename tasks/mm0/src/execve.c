/*
 * Program execution
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/ipcdefs.h>
#include <l4lib/types.h>
#include <l4/macros.h>
#include <l4/api/errno.h>
#include <lib/malloc.h>
#include <vm_area.h>
#include <syscalls.h>
#include <string.h>
#include <file.h>
#include <user.h>
#include <task.h>

/*
 * Different from vfs_open(), which validates an already opened
 * file descriptor, this call opens a new vfs file by the pager
 * using the given path. The vnum handle and file length is returned
 * since the pager uses this information to access file pages.
 */
int vfs_open_bypath(const char *pathname, unsigned long *vnum, unsigned long *length)
{
	int err = 0;
	struct tcb *vfs;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	if (!(vfs = find_task(VFS_TID)))
		return -ESRCH;

	/*
	 * Copy string to vfs utcb.
	 *
	 * FIXME: There's a chance we're overwriting other tasks'
	 * ipc information that is on the vfs utcb.
	 */
	strcpy(vfs->utcb, pathname);

	l4_save_ipcregs();

	write_mr(L4SYS_ARG0, (unsigned long)vfs->utcb);

	if ((err = l4_sendrecv(VFS_TID, VFS_TID,
			       L4_IPC_TAG_PAGER_OPEN_BYPATH)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		goto out;
	}

	/* Check if syscall was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: VFS open error: %d.\n",
		       __FUNCTION__, err);
		goto out;
	}

	/* Read file information */
	*vnum = read_mr(L4SYS_ARG0);
	*length = read_mr(L4SYS_ARG1);

out:
	l4_restore_ipcregs();
	return err;
}


int do_execve(char *filename)
{
	int err;
	unsigned long vnum, length;
	struct vm_file *f;

	/* Get file info from vfs */
	if ((err = vfs_open_bypath(filename, &vnum, &length)) < 0)
		return err;

	/* Create and get the file structure */
	if (IS_ERR(f = do_open2(0, 0, vnum, length)))
		return (int)f;


	/* Determine file segments to be mapped */

	/* See if an interpreter (dynamic linker) is needed */

	/* Destroy all threads in the same thread group except group leader */

	/* Release all task resources, do almost everything done in exit() */

	/*
	 * Create new process address space. Start by mapping all
	 * static file segments. We will need brk() for bss.
	 */


#if 0
TODO:
Dynamic Linking.

	/* Find the interpreter executable file, if needed */

	/*
	 * Map all dynamic linker file segments
	 * (should not clash with original executable
	 */

	/* Set up registers to run dynamic linker (exchange_registers()) */

	/* Run the interpreter */

	/*
	 * The interpreter:
	 * - May need some initial info (dyn sym tables) at a certain location
	 * - Will find necessary shared library files in userspace
	 *   (will use open/read).
	 * - Map them into process address space via mmap()
	 * - Reinitialise references to symbols in the shared libraries
	 * - Jump to the entry point of main executable.
	 */
#endif
	return -1;
}


/*
 * Copies a null-terminated ragged array (i.e. argv[0]) from userspace into
 * buffer. If any page boundary is hit, unmaps previous page, validates and
 * maps the new page.
 */
int copy_user_ragged(struct tcb *task, char *buf[], char *user[], int maxlength)
{
	return 0;
}

/*
 * Copy from one buffer to another. Stop if maxlength or
 * a page boundary is hit.
 */
int strncpy_page(char *to, char *from, int maxlength)
{
	int count = 0;

	do {
		if ((to[count] = from[count]) == '\0')
			break;
		count++;
	} while (count < maxlength && !page_boundary(&from[count]));

	if (page_boundary(&from[count]))
		return -EFAULT;
	if (count == maxlength)
		return -E2BIG;

	return 0;
}

/*
 * Copies a userspace string into buffer. If a page boundary is hit,
 * unmaps the previous page, validates and maps the new page
 */
int copy_user_string(struct tcb *task, char *buf, char *user, int maxlength)
{
	int count = maxlength;
	char *mapped = 0;
	int copied = 0;
	int err = 0;

	/* Map the first page the user buffer is in */
	if (!(mapped = pager_validate_map_user_range(task, user,
						     TILL_PAGE_ENDS(user),
						     VM_READ)))
		return -EINVAL;

	while ((err = strncpy_page(&buf[copied], mapped, count)) < 0) {
		if (err == -E2BIG)
			return err;
		if (err == -EFAULT) {
			pager_unmap_user_range(mapped, TILL_PAGE_ENDS(mapped));
			copied += TILL_PAGE_ENDS(mapped);
			count -= TILL_PAGE_ENDS(mapped);
			if (!(mapped =
			      pager_validate_map_user_range(task, user + copied,
							    TILL_PAGE_ENDS(user + copied),
							    VM_READ)))
				return -EINVAL;
		}
	}

	/* Unmap the final page */
	pager_unmap_user_range(mapped, TILL_PAGE_ENDS(mapped));

	return 0;
}

int sys_execve(struct tcb *sender, char *pathname, char *argv[], char *envp[])
{
	int err;
	char *path = kzalloc(PATH_MAX);

	/* Copy the executable path string */
	if ((err = copy_user_string(sender, path, pathname, PATH_MAX)) < 0)
		return err;
	printf("%s: Copied pathname: %s\n", __FUNCTION__, path);

	return do_execve(path);
}



