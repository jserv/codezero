#ifndef __KMALLOC_H__
#define __KMALLOC_H__

void *kmalloc(int size);
int kfree(void *p);
void *kzalloc(int size);
void init_kmalloc();

#endif
