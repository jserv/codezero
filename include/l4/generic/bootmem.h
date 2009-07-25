/*
 * Copyright (C) 2009 Bahadir Balban
 */

#ifndef __BOOTMEM_H__
#define __BOOTMEM_H__

void *alloc_bootmem(int size, int alignment);
pmd_table_t *alloc_boot_pmd(void);

extern pgd_table_t init_pgd;

#endif /* __BOOTMEM_H__ */
