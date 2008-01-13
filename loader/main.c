/*******************************************************************************
* Filename:    src/main.c                                                      *
* Description: Elf-loader - ELF file kernel/application bootstraper, main      *
*              source file.                                                    *
* Authors:     Adam 'WeirdArms' Wiggins <awiggins@cse.unsw.edu.au>.            *
* Created:     2004-12-01                                                      *
********************************************************************************
*                                                                              *
* Australian Public Licence B (OZPLB)                                          *
*                                                                              *
* Version 1-0                                                                  *
*                                                                              *
* Copyright (c) 2004 - 2005 University of New South Wales, Australia           *
*                                                                              *
* All rights reserved.                                                         *
*                                                                              *
* Developed by: Operating Systems, Embedded and                                *
*               Distributed Systems Group (DiSy)                               *
*               University of New South Wales                                  *
*               http://www.disy.cse.unsw.edu.au                                *
*                                                                              *
* Permission is granted by University of New South Wales, free of charge, to   *
* any person obtaining a copy of this software and any associated              *
* documentation files (the "Software") to deal with the Software without       *
* restriction, including (without limitation) the rights to use, copy,         *
* modify, adapt, merge, publish, distribute, communicate to the public,        *
* sublicense, and/or sell, lend or rent out copies of the Software, and        *
* to permit persons to whom the Software is furnished to do so, subject        *
* to the following conditions:                                                 *
*                                                                              *
*     * Redistributions of source code must retain the above copyright         *
*       notice, this list of conditions and the following disclaimers.         *
*                                                                              *
*     * Redistributions in binary form must reproduce the above                *
*       copyright notice, this list of conditions and the following            *
*       disclaimers in the documentation and/or other materials provided       *
*       with the distribution.                                                 *
*                                                                              *
*     * Neither the name of University of New South Wales, nor the names of    *
*        its contributors, may be used to endorse or promote products derived  *
*       from this Software without specific prior written permission.          *
*                                                                              *
* EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT            *
* PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND           *
* NATIONAL ICT AUSTRALIA AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS,         *
* WARRANTIES OR CONDITIONS OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING          *
* BUT NOT LIMITED TO ANY REPRESENTATIONS, WARRANTIES OR CONDITIONS             *
* REGARDING THE CONTENTS OR ACCURACY OF THE SOFTWARE, OR OF TITLE,             *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,          *
* THE ABSENCE OF LATENT OR OTHER DEFECTS, OR THE PRESENCE OR ABSENCE OF        *
* ERRORS, WHETHER OR NOT DISCOVERABLE.                                         *
*                                                                              *
* TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL            *
* NATIONAL ICT AUSTRALIA OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL            *
* THEORY (INCLUDING, WITHOUT LIMITATION, IN AN ACTION OF CONTRACT,             *
* NEGLIGENCE OR OTHERWISE) FOR ANY CLAIM, LOSS, DAMAGES OR OTHER               *
* LIABILITY, INCLUDING (WITHOUT LIMITATION) LOSS OF PRODUCTION OR              *
* OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA OR RECORDS; OR LOSS       *
* OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT OR GOODWILL, OR         *
* OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL, INDIRECT,                   *
* CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF OR IN           *
* CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER            *
* DEALINGS WITH THE SOFTWARE, EVEN IF NATIONAL ICT AUSTRALIA OR ITS            *
* CONTRIBUTORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS,       *
* DAMAGES OR OTHER LIABILITY.                                                  *
*                                                                              *
* If applicable legislation implies representations, warranties, or            *
* conditions, or imposes obligations or liability on University of New South   *
* Wales or one of its contributors in respect of the Software that             *
* cannot be wholly or partly excluded, restricted or modified, the             *
* liability of University of New South Wales or the contributor is limited, to *
* the full extent permitted by the applicable legislation, at its              *
* option, to:                                                                  *
* a.  in the case of goods, any one or more of the following:                  *
* i.  the replacement of the goods or the supply of equivalent goods;          *
* ii.  the repair of the goods;                                                *
* iii. the payment of the cost of replacing the goods or of acquiring          *
*  equivalent goods;                                                           *
* iv.  the payment of the cost of having the goods repaired; or                *
* b.  in the case of services:                                                 *
* i.  the supplying of the services again; or                                  *
* ii.  the payment of the cost of having the services supplied again.          *
*                                                                              *
* The construction, validity and performance of this licence is governed       *
* by the laws in force in New South Wales, Australia.                          *
*                                                                              *
*******************************************************************************/

#include <elf/elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "arch.h"

/*****************
* Extern symbols *
*****************/

/* These symbols are defined by the linker scripts. */
extern char _start_kernel[];
extern char _end_kernel[];
extern char _start_mm0[];
extern char _end_mm0[];
extern char _start_fs0[];
extern char _end_fs0[];
extern char _start_test0[];
extern char _end_test0[];
extern char _start_bootdesc[];
extern char _end_bootdesc[];

/* This is a kernel symbol exported to loader's linker script from kernel build */
extern char bkpt_phys_to_virt[];

/*****************************
* Local function prototypes. *
*****************************/

static void load_image(void **entry, char *start, char *end);

int
main(void)
{
	void *kernel_entry = NULL;
	void *mm0_entry = NULL;
	void *fs0_entry = NULL;
	void *test0_entry = NULL;
	void *bootdesc_entry = NULL;
	arch_init();

	printf("elf-loader:\tStarted\n");
	printf("Loading the kernel...\n");
	load_image(&kernel_entry, _start_kernel, _end_kernel);
	printf("Loading the inittask\n");
	load_image(&mm0_entry, _start_mm0, _end_mm0);
	printf("Loading the roottask\n");
	load_image(&fs0_entry, _start_fs0, _end_fs0);
	printf("Loading the testtask\n");
	load_image(&test0_entry, _start_test0, _end_test0);
	printf("Loading the bootdesc\n");
	load_image(&bootdesc_entry, _start_bootdesc, _end_bootdesc);

	printf("elf-loader:\tkernel entry point is %p\n", kernel_entry);
	arch_start_kernel(kernel_entry);

	printf("elf-loader:\tKernel start failed!\n");

	return -1;
}

/*******************
* Local functions. *
*******************/

static void
load_image(void** entry, char *start, char *end)
{
	if(start == end) {
		printf("elf-loader: error, dite image is empty!\n");
		return;
	}
	/*  awiggins (2004-12-21): Should probably add an alignment check? */
        /** awiggins (2005-05-29): Probably should add checks to see 
         *  the correct kind of elf file is being loaded for the platform,
         *  or does the elf library do that somewhere?.
         */
	if(!elf32_checkFile((struct Elf32_Header*)start)) {
        //        printf("32-bit elf image detected.\n");
		*entry = (void*)(uintptr_t)
				elf32_getEntryPoint((struct Elf32_Header*)start);
		printf("Entry point: %p\n", *entry);
	} else if(!elf64_checkFile((struct Elf64_Header*)start)) {
		*entry =
                        (void*)(uintptr_t)elf64_getEntryPoint((struct Elf64_Header*)start);
	} else {
		printf("elf-loader: error, dite image not a valid elf file!\n");
		return;
	}
#ifdef __PPC64__
	if (arch_claim_memory((struct Elf32_Header*)start)) {
		printf("could not claim memory for elf file!\n");
		return;
	}
#endif
	//printf("Start: %p\n", start);
	//printf("End: %p\n", end);

	if(!elf_loadFile(start, true)) {
		printf("elf-loader: error, unable to load dite image!\n");
		return;
	}
}

