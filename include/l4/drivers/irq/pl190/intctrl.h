/*********************************************************************
 *                
 * Copyright (C) 2004,  National ICT Australia (NICTA)
 *                
 * File path:     platform/pleb2/intctrl.h
 * Description:   
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: intctrl.h,v 1.2 2004/12/02 22:40:44 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __PLATFORM__PB926__INTCTRL_H__
#define __PLATFORM__PB926__INTCTRL_H__

#include <intctrl.h>
#include INC_GLUE(hwspace.h)
#include INC_ARCH(thread.h)
#include INC_ARCH(clz.h)
#include INC_API(space.h)
#include INC_PLAT(offsets.h)
#include INC_PLAT(timer.h)
#include INC_CPU(cpu.h)

/* Flags to define Primary and Secondary irq controllers */
#define PB926_PIC		0
#define PB926_SIC		1

/* Flags to define fast or normal interrupts */
#define PB926_IRQ		0
#define PB926_FIQ		1


void vic_init(void);

/* IRQS */
#define IRQS			64

/* FIXME:
 * IRQ handler array doesn't match with irq descriptor array, which must be taken as
 * reference because its indices are unique. Make sure this happens.
 */
/* Our major OS timer */
#define PB926_IRQ_OS_TIMER

/* Virtual offsets from some virtual device base */

/* Absolute virtual addresses */
#define	PB926_VIC_VBASE		(IODEVICE_VADDR + PB926_VIC_VOFFSET)
#define	PB926_SIC_VBASE		(IODEVICE_VADDR + PB926_SIC_VOFFSET)

/* Interrupt controller register offsets (absolute virtual) */
#define PIC_IRQSTATUS			(volatile word_t *)(PB926_VIC_VBASE + 0x00)
#define PIC_FIQSTATUS			(volatile word_t *)(PB926_VIC_VBASE + 0x04)
#define PIC_RAWINTR			(volatile word_t *)(PB926_VIC_VBASE + 0x08)
#define PIC_INTSELECT			(volatile word_t *)(PB926_VIC_VBASE + 0x0C)
#define PIC_INTENABLE			(volatile word_t *)(PB926_VIC_VBASE + 0x10)
#define PIC_INTENCLEAR			(volatile word_t *)(PB926_VIC_VBASE + 0x14)
#define PIC_SOFTINT			(volatile word_t *)(PB926_VIC_VBASE + 0x18)
#define PIC_SOFTINTCLEAR		(volatile word_t *)(PB926_VIC_VBASE + 0x1C)
#define PIC_PROTECTION			(volatile word_t *)(PB926_VIC_VBASE + 0x20)
#define PIC_VECTADDR			(volatile word_t *)(PB926_VIC_VBASE + 0x30)
#define PIC_DEFVECTADDR			(volatile word_t *)(PB926_VIC_VBASE + 0x34)
#define PIC_VECTADDR0			(volatile word_t *)(PB926_VIC_VBASE + 0x100)
/* 15 PIC_VECTADDR registers up to	0x13C */
#define PIC_VECTCNTL0			(volatile word_t *)(PB926_VIC_VBASE + 0x200)
/* 15 PIC_VECTCNTL registers up to	0x23C */

#define SIC_STATUS			(volatile word_t *)(PB926_SIC_VBASE + 0x0)
#define SIC_RAWSTAT			(volatile word_t *)(PB926_SIC_VBASE + 0x04)
#define SIC_ENABLE			(volatile word_t *)(PB926_SIC_VBASE + 0x08)
#define SIC_ENSET			(volatile word_t *)(PB926_SIC_VBASE + 0x08)
#define SIC_ENCLR			(volatile word_t *)(PB926_SIC_VBASE + 0x0C)
#define SIC_SOFTINTSET			(volatile word_t *)(PB926_SIC_VBASE + 0x10)
#define SIC_SOFTINTCLR			(volatile word_t *)(PB926_SIC_VBASE + 0x14)
#define SIC_PICENABLE			(volatile word_t *)(PB926_SIC_VBASE + 0x20)
#define SIC_PICENSET			(volatile word_t *)(PB926_SIC_VBASE + 0x20)
#define SIC_PICENCLR			(volatile word_t *)(PB926_SIC_VBASE + 0x24)

/*******************************************/


	/* BB:  IRQ Explanation for VIC
	 * On PB926 there are two interrupt controllers. A secondary IC is 
	 * cascaded on the primary, using pin 31. We ack irqs by simply clearing
	 * the status register. We actually ack_and_mask, so for acking, we also 
	 * disable the particular irq, on either pic or sic.
	 * 
	 * What's more to cascaded irq controllers is that, you may have the 
	 * same line number on different controllers, for different irqs. This
	 * causes a difficulty to uniquely identify an interrupt, because the 
	 * line number is not a primary key anymore. One approach is to `assume'
	 * irqs on one controller are added to the maximum you can have on the 
	 * other. E.g. irq 1 on intctrl P is 1, but irq 1 on intctrl S is 
	 * (1 + 31) = 32. While this works, it's not the best approach because 
	 * you will see that more complicated architectures on SMP machines are 
	 * present. (See PB1176 or EB port, or even worse, try an ARM11MPCore 
	 * on EB.)
	 * 
	 * What's better is to use a struct to describe what the irq line 
	 * `actually' is, so things are put straight, with a small memory 
	 * trade-off.
	 * 
	 * Furthermore to this, each irq is described uniquely by an irq
	 * descriptor; however, we still need the simplicity of a single integer
	 * to uniquely identify each individual irq. This integer, is the index
	 * of each irq descriptor, in the array of irq descriptors. This index
	 * is not necessarily a natural mapping from irq numbers in interrupt
	 * controllers, as described in paragraph 2, but it is rather a compile-
	 * time constant as far as the compile-time defined irq array is
	 * concerned, or, the set of such primary keys may expand as the array
	 * expands dynamically at run-time (e.g. registration of new irqs at
	 * run-time).
	 */

extern word_t arm_high_vector; 
extern word_t interrupt_handlers[IRQS];
class intctrl_t : public generic_intctrl_t {
	
/* Timer needs to know about its irq, stored in this class. */
friend class arm_sp804timer;

/* Defines an irq on pb926 */
public:
struct pb926dev_irq {
	int intctrl;	/* The irq controller this irq is on */
	int irqno;	/* The pin (i.e. line) on this controller */
	int irqtype;	/* Fast, or normal. IRQ = 0, FIQ = 1 */
};

/* TODO: The reason our static variables aren't static anymore is that,
 * they need to be instantiated and initialised in global scope, in order to be
 * used. E.g:
 * intctrl_t::pb926_irq_watchdog = { 1, 3, 4 }; 
 * I haven't figured out where and how to initialise them this way yet, thus
 * they're non-static for now. 
 */

#if 0
/* IRQ definitions */
static struct pb926dev_irq pb926_irq_watchdog = {
	//.intctrl= PB926_PIC,
	//.irqno	= 0,
	//.irqtype= PB926_IRQ
PB926_PIC, 0, PB926_IRQ
};
static struct pb926dev_irq pb926_irq_timer0_1 = {
//	.intctrl= PB926_PIC,
//	.irqno	= 4,
//	.irqtype= PB926_IRQ
PB926_PIC, 4, PB926_IRQ
};
#endif

struct pb926dev_irq pb926_irq_watchdog;
struct pb926dev_irq pb926_irq_timer0_1;
struct pb926dev_irq pb926_irq_uart0;
struct pb926dev_irq * irq_desc_array[IRQS];

public:

void init_cpu();

void init_arch(void)
{
	pb926_irq_watchdog.intctrl = PB926_PIC;
	pb926_irq_watchdog.irqno   = 0;
	pb926_irq_watchdog.irqtype = PB926_IRQ;
	
	pb926_irq_timer0_1.intctrl = PB926_PIC;
	pb926_irq_timer0_1.irqno   = 4;
	pb926_irq_timer0_1.irqtype = PB926_IRQ;
	
	pb926_irq_uart0.intctrl = PB926_PIC;
	pb926_irq_uart0.irqno   = 12;
	pb926_irq_uart0.irqtype = PB926_IRQ;

	irq_desc_array[0] = &pb926_irq_watchdog;
	irq_desc_array[4] = &pb926_irq_timer0_1;
	irq_desc_array[12]= &pb926_irq_uart0;

	vic_init();
}

int irq_number(void);
void ack(int irqno);
void mask(int irqno);
int unmask(int irq);
bool is_irq_available(int irq);

word_t get_number_irqs(void)
{
	return IRQS;
}

void register_interrupt_handler (word_t vector, void (*handler)(word_t,
	arm_irq_context_t *))
{
	ASSERT(DEBUG, vector < IRQS);
	interrupt_handlers[vector] = (word_t) handler;
	TRACE_INIT("interrupt vector[%d] = %p\n", vector, 
	interrupt_handlers[vector]);
}


inline void disable(int irq)
{  
	mask(irq);
}

inline int enable(int irq)
{
	return unmask(irq);
}

void disable_fiq(void) {}


void set_cpu(word_t irq, word_t cpu) {}

}; 
#endif /*__PLATFORM__PB926__INTCTRL_H__ */
