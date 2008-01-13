/*
 * Test shmget/shmat/shmdt posix calls.
 *
 * Copyright (C) 2007 - 2008 Bahadir Balban
 */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <tests.h>

int shmtest(void)
{
	//key_t keys[2] = { 5, 10000 };
	key_t keys[2] = { 2, 3 };
	void *bases[2] = { 0 , 0 };
	int shmids[2];

	printf("Initiating shmget()\n");
	for (int i = 0; i < 2; i++) {
		if ((shmids[i] = shmget(keys[i], 27, IPC_CREAT | 0666)) < 0) {
			printf("Call failed.\n");
			perror("SHMGET");
		} else
			printf("SHMID returned: %d\n", shmids[i]);
	}
	printf("Now shmat()\n");
	for (int i = 0; i < 2; i++) {
		if ((int)(bases[i] = shmat(shmids[i], NULL, 0)) == -1)
			perror("SHMAT");
		else
			printf("SHM base address returned: %p\n", bases[i]);
	}
	printf("Now shmdt()\n");
	for (int i = 0; i < 2; i++) {
		if (shmdt(bases[i]) < 0)
			perror("SHMDT");
		else
			printf("SHM detached OK.\n");
	}
	printf("Now shmat() again\n");
	for (int i = 0; i < 2; i++) {
		if ((int)(bases[i] = shmat(shmids[i], NULL, 0)) == -1)
			perror("SHMAT");
		else
			printf("SHM base address returned: %p\n", bases[i]);
	}

	return 0;
}
