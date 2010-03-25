#ifndef __IPI_H__
#define __IPI_H__

/*
 * Copyright 2010 B Labs.Ltd.
 *
 * Author: Prem Mallappa  <prem.mallappa@b-labs.co.uk>
 *
 * Description: 
 */


#include <l4/generic/irq.h>

int ipi_handler(struct irq_desc *desc);


#endif	/* __IPI_H__ */
