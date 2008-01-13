

/* This is used to place a binary file into an elf section */
.section .data.fs

.incbin "fsbin/romfs.bin"
.align 4
