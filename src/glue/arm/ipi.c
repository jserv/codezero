/*
 * Copyright 2010 B Labs.Ltd.
 *
 * Author: Prem Mallappa  <prem.mallappa@b-labs.co.uk>
 *
 * Description: IPI handler for all ARM SMP cores
 */

#include INC_GLUE(ipi.h)
#include INC_GLUE(smp.h)
#include INC_SUBARCH(cpu.h)
#include <l4/lib/printk.h>

/* This should be in a file something like exception.S */
int ipi_handler(struct irq_desc *desc)
{
        return 0;
}
