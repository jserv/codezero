#ifndef __LIB_STRING_H__
#define __LIB_STRING_H__

int strlen(const char *s);
char *strcpy(char *to, const char *from);
char *strncpy(char *dest, const char *src, int count);
int strcmp(const char *s1, const char *s2);
void *memset(void *p, int c, int size);
void *memcpy(void *d, void *s, int size);

#endif /* __LIB_STRING_H__ */
