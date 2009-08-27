
#ifndef __PRINTF_H__
#define __PRINTF_H__

#include <stdarg.h>
#include <stdio.h>
int printf(char *format, ...) __attribute__((format (printf, 1, 2)));

#endif /* __PRINTF_H__ */
