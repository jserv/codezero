/*
 * This is for testing utcb port from POSIX
 */
#include <stdio.h>
#include <utcb.h>

void utcb_test(unsigned long utcb_start, unsigned long utcb_end)
{
	struct utcb_desc *udesc[20];
	unsigned long addr;
	int idx = -1;

	printf("%s\n", __FUNCTION__);

	utcb_pool_init(utcb_start, utcb_end);

	// Test-1
	for (int i = 0; i < 10; i++)
		udesc[++idx] = utcb_new_desc();
	for (int j = 0; j < 20; ++j)
		addr = utcb_new_slot(udesc[idx]);

	// Test 2
	for (int i = 0; i < 3; i++)
		udesc[++idx] = utcb_new_desc();
	for (int j = 0; j < 10; ++j)
		addr = utcb_new_slot(udesc[idx]);
	for (int j = 0; j < 8; j++) {
		utcb_delete_slot(udesc[idx], addr);
		addr -= 0x100;
	}
	for (int j = 0; j < 2; ++j)
		addr = utcb_new_slot(udesc[idx]);
	for (int j = 0; j < 4; j++) {
		utcb_delete_slot(udesc[idx], addr);
		addr -= 0x100;
	}
	// Test 3
	for (int i = 0; i < 7; i++) {
		utcb_delete_desc(udesc[idx]);
		--idx;
	}
	for (int j = 0; j < 4; ++j)
		addr = utcb_new_slot(udesc[idx]);
	for (int j = 0; j < 2; j++) {
		utcb_delete_slot(udesc[idx], addr);
		addr -= 0x100;
	}
	for (int i = 0; i < 3; i++)
		udesc[++idx] = utcb_new_desc();
	for (int i = 0; i < 7; i++) {
		utcb_delete_desc(udesc[idx]);
		--idx;
	}
}
