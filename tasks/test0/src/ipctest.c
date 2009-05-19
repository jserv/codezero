#include <l4lib/arch/syslib.h>
#include <l4lib/ipcdefs.h>

/*
 * Full ipc test. Sends/receives full utcb, done with the pager.
 */
void ipc_full_test(void)
{
	int ret = 0;

	/* Fill in all of the utcb locations */
	for (int i = MR_UNUSED_START; i < MR_TOTAL + MR_REST; i++) {
		//printf("Writing: MR%d: %d\n", i, i);
		write_mr(i, i);
	}

	/* Call the pager */
	if ((ret = l4_sendrecv_full(PAGER_TID, PAGER_TID, L4_IPC_TAG_SYNC_FULL)) < 0) {
		printf("%s: Failed with %d\n", __FUNCTION__, ret);
		BUG();
	}
	/* Read back updated utcb */
	for (int i = MR_UNUSED_START; i < MR_TOTAL + MR_REST; i++) {
		//printf("Read MR%d: %d\n", i, read_mr(i));
		if (read_mr(i) != 0) {
			printf("Expected 0 on all mrs. Failed.\n");
			BUG();
		}
	}

	if (!ret)
		printf("FULL IPC TEST: -- PASSED --\n");
	else
		printf("FULL IPC TEST: -- FAILED --\n");
}

