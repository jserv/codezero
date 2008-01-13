#ifndef __PGALLOC_H__
#define __PGALLOC_H__

void *zalloc_page(void);
void *alloc_page(void);
void *alloc_pmd(void);
void *alloc_pgd(void);
int free_page(void *);
int free_pmd(void *);
int free_pgd(void *);

int pgalloc_add_new_grant(unsigned long pfn, int npages);
void init_pgalloc();

#endif /* __PGALLOC_H__ */
