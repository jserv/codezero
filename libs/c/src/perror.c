/*
 * A temporary mock-up perror implementation just
 * for quick testing purposes. When a proper C library is
 * ported, its implementation should be used.
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <stdio.h>

extern int errno;

void perror(const char *str)
{
	printf("%s: %d\n", str, errno);
}
