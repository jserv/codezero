/*
 * These functions here do run-time checks on all fields
 * of tasks, vmas, and vm objects to see that they
 * have expected values.
 *
 * Copyright (C) 2008 Bahadir Balban
 */

#include <vm_area.h>
#include <mmap.h>
#include <shm.h>
#include <globals.h>

struct vm_statistics {
	int tasks;
	int vm_objects;
	int shadow_objects;	/* Shadows counted by hand (well almost!) */
	int shadows_referred;	/* Shadows that objects say they have */
	int file_objects;
	int vm_files;
	int tasks_total;
};

/* Count links in objects link list, and compare with nlinks */
int vm_object_test_link_count(struct vm_object *vmo)
{
	int links = 0;
	struct vm_obj_link *l;

	list_for_each_entry(l, &vmo->link_list, linkref)
		links++;

	BUG_ON(links != vmo->nlinks);
	return 0;
}

int vm_object_test_shadow_count(struct vm_object *vmo)
{
	struct vm_object *sh;
	int shadows = 0;

	list_for_each_entry(sh, &vmo->shdw_list, shref)
		shadows++;

	BUG_ON(shadows != vmo->shadows);
	return 0;
}

int mm0_test_global_vm_integrity(void)
{
	struct tcb *task;
	struct vm_object *vmo;
	struct vm_statistics vmstat;
	struct vm_file *f;


	memset(&vmstat, 0, sizeof(vmstat));

	/* Count all shadow and file objects */
	list_for_each_entry(vmo, &global_vm_objects.list, list) {
		vmstat.shadows_referred += vmo->shadows;
		if (vmo->flags & VM_OBJ_SHADOW)
			vmstat.shadow_objects++;
		if (vmo->flags & VM_OBJ_FILE)
			vmstat.file_objects++;
		vmstat.vm_objects++;
		vm_object_test_shadow_count(vmo);
		vm_object_test_link_count(vmo);
	}

	/* Count all registered vmfiles */
	list_for_each_entry(f, &global_vm_files.list, list)
		vmstat.vm_files++;

	if (vmstat.vm_files != global_vm_files.total) {
		printf("Total counted files don't match "
		       "global_vm_files total\n");
		BUG();
	}

 	if (vmstat.vm_objects != global_vm_objects.total) {
		printf("Total counted vm_objects don't "
		       "match global_vm_objects total\n");
		BUG();
	}

	/* Total file objects must be equal to total vm files */
	if (vmstat.vm_files != vmstat.file_objects) {
		printf("\nTotal files don't match total file objects.\n");
		printf("vm files:\n");
		vm_print_files(&global_vm_files.list);
		printf("\nvm objects:\n");
		vm_print_objects(&global_vm_objects.list);
		printf("\n");
		BUG();
	}

	/* Counted and referred shadows must match */
	BUG_ON(vmstat.shadow_objects != vmstat.shadows_referred);

	/* Count all tasks */
	list_for_each_entry(task, &global_tasks.list, list)
		vmstat.tasks++;

 	if (vmstat.tasks != global_tasks.total) {
		printf("Total counted tasks don't match global_tasks total\n");
		BUG();
	}
	return 0;
}



