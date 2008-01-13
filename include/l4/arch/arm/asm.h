/*********************************************************************
 *                
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     arch/arm/asm.h
 * Description:   Assembler macros etc. 
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
 * $Id: asm.h,v 1.4 2004/12/02 00:15:07 cvansch Exp $
 *                
 ********************************************************************/

#ifndef __ARCH_ARM_ASM_H__
#define __ARCH_ARM_ASM_H__


/* Top nibble of the byte denotes irqs/fiqs disabled, ARM state */
#define ARM_MODE_MASK	0x1F

#define	ARM_MODE_SVC	0x13
#define ARM_MODE_UND	0x1B
#define ARM_MODE_ABT	0x17
#define ARM_MODE_IRQ	0x12
#define ARM_MODE_FIQ	0x11
#define ARM_MODE_USR	0x10
#define ARM_MODE_SYS	0x1F
#define	ARM_NOIRQ_SVC	0xD3
#define ARM_NOIRQ_UND	0xDB
#define ARM_NOIRQ_ABT	0xD7
#define ARM_NOIRQ_IRQ	0xD2
#define ARM_NOIRQ_FIQ	0xD1
#define ARM_NOIRQ_USR	0xD0
#define ARM_NOIRQ_SYS	0xDF
/* For enabling *clear* these bits */
#define ARM_IRQ_BIT	0x80
#define ARM_FIQ_BIT	0x40

/* Notes about ARM instructions:
 *
 * TST instruction:
 *
 * Essentially TST "AND"s two values and the result affects the Z (Zero bit)
 * in CPSR, which can be used for conditions. For example in:
 *
 * 	TST r0, #VALUE
 *
 * If anding r0 and #VALUE results in a positive value (i.e. they have a
 * common bit set as 1) then Z bit is 0, which accounts for an NE (Not equal)
 * condition. Consequently, e.g. a BEQ instruction would be skipped and a BNE
 * would be executed.
 *
 * In the opposite case, r0 and #VALUE has no common bits, and anding them
 * results in 0. This means Z bit is 1, and any EQ instruction coming afterwards
 * would be executed.
 *
 * I have made this explanation here because I think the behaviour of the Z bit
 * is kind of hacky in TST. Normally Z bit is used for equivalence (e.g. CMP
 * instruction) but in TST case even if two values were equal the Z bit could
 * point to an NE or EQ condition depending on whether the values have non-zero
 * bits.
 */


#define dbg_stop_here()		__asm__ __volatile__ (	"bkpt	#0\n" :: )

#define BEGIN_PROC(name)			\
    .global name; 				\
    .type   name,function;			\
    .align;					\
name:

#define END_PROC(name)				\
.fend_##name:					\
    .size   name,.fend_##name - name;

/* Functions to generate symbols in the output file
 * with correct relocated address for debugging
 */
#define	TRAPS_BEGIN_MARKER			\
    .balign 4096;				\
    .section .data.traps;			\
    __vector_vaddr:

#define VECTOR_WORD(name)			\
    .equ    vector_##name, (name - __vector_vaddr + 0xffff0000);	\
    .global vector_##name;			\
    .type   vector_##name,object;		\
    .size   vector_##name,4;			\
name:

/* NOTE: These are fairly useless vector relocation macros. Why? Because
 * ARM branch instructions are *relative* anyway. */
#define BEGIN_PROC_TRAPS(name)			\
    .global name; 				\
    .type   name,function;			\
    .equ    vector_##name, (name - __vector_vaddr + 0xffff0000);	\
    .global vector_##name;			\
    .type   vector_##name,function;		\
    .align;					\
name:

#define END_PROC_TRAPS(name)			\
.fend_##name:					\
    .equ    .fend_vector_##name, (.fend_##name - __vector_vaddr + 0xffff0000);	\
    .size   name,.fend_##name - name;		\
    .size   vector_##name,.fend_vector_##name - vector_##name;

#define BEGIN_PROC_KIP(name)			\
    .global name; 				\
    .type   name,function;			\
    .align;					\
name:

#define END_PROC_KIP(name)			\
.fend_##name:					\
    .size   name,.fend_##name - name;

#define CHECK_ARG(a, b)				\
	"   .ifnc " a ", " b "	\n"		\
	"   .err		\n"		\
	"   .endif		\n"		\

#endif /* __ARCH_ARM_ASM_H__ */
