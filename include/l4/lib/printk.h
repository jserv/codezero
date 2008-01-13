
#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>

#if defined(ARCH_TEST)
/* For host tests all printks mean printf using the host C library */
#include <stdio.h>
#define printk		printf
#elif !defined(__KERNEL__)
#define	printk 		printf
#else
int printk(char *format, ...) __attribute__((format (printf, 1, 2)));
extern void putc(char c);
#endif

#endif /* __PRINTK_H__ */
