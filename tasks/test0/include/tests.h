#ifndef __TEST0_TESTS_H__
#define __TEST0_TESTS_H__

#define __TASKNAME__			"test0"

//#define TEST_VERBOSE_PRINT
#if defined (TEST_VERBOSE_PRINT)
#define test_printf(...)	printf(__VA_ARGS__)
#else
#define test_printf(...)
#endif

#include <sys/types.h>
extern pid_t parent_of_all;

int shmtest(void);
int forktest(void);
int mmaptest(void);
int dirtest(void);
int fileio(void);
int clonetest(void);
int exectest(void);

#endif /* __TEST0_TESTS_H__ */
