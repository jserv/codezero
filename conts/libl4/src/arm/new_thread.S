#include <l4lib/arch/asm.h>


BEGIN_PROC(setup_new_thread)
	ldr	r0, [sp], #-4		@ Load first argument
	mov	lr, pc			@ Save return address
	ldr	pc, [sp], #-4		@ Load function pointer from stack
	b	l4_thread_exit		@ Call l4_thread_exit for cleanup
new_thread_exit:
	b	new_thread_exit		@ Never reaches here
END_PROC(setup_new_thread)

