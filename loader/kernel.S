
/* This file defines kernel symbols extracted from the kernel image in their physical address */
.include "start.axf.S"

.section .kernel

.incbin "start.axf"
.align 4
.section .mm0

.incbin "mm0.axf"
.align 4
.section .bootdesc

.incbin "bootdesc.axf"

.section .fs0

.incbin "fs0.axf"
.align 4

.section .test0

.incbin "test0.axf"
.align 4
