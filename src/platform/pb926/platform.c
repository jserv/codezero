/*
 * PB926 platform-specific initialisation and setup
 *
 * Copyright (C) 2007 Bahadir Balban
 */
#include <l4/generic/platform.h>
#include <l4/generic/space.h>
#include <l4/generic/irq.h>
#include <l4/generic/bootmem.h>
#include INC_ARCH(linker.h)
#include INC_SUBARCH(mm.h)
#include INC_SUBARCH(mmu_ops.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(mapping.h)
#include INC_GLUE(memlayout.h)
#include INC_PLAT(offsets.h)
#include INC_PLAT(platform.h)
#include INC_PLAT(uart.h)
#include INC_PLAT(timer.h)
#include INC_PLAT(irq.h)
#include INC_ARCH(asm.h)
#include INC_ARCH(io.h)
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>

struct platform_mem_regions platform_mem_regions = {
	.nregions = 1,
	.mem_range = {
		[0] = {
		.start = PLATFORM_PHYS_MEM_START,
		.end = PLATFORM_PHYS_MEM_END,
		.type = MEM_TYPE_RAM,
		},
		[1] = {
		.start = PLATFORM_SYSTEM_REGISTERS,
		.end = PLATFORM_SYSTEM_REGISTERS + PLATFORM_SYSREGS_SIZE,
		.type = MEM_TYPE_DEV,
		},
	},
};

/*
 * We will use UART0 for kernel as well as user tasks,
 * so map it to kernel and user space
 */
void init_platform_console(void)
{
	add_boot_mapping(PLATFORM_UART0_BASE, PLATFORM_CONSOLE_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);

	/*
	 * Map same UART IO area to userspace so that primitive uart-based
	 * userspace printf can work. Note, this raw mapping is to be
	 * removed in the future, when file-based io is implemented.
	 */
	add_boot_mapping(PLATFORM_UART0_BASE, USERSPACE_CONSOLE_VBASE,
			 PAGE_SIZE, MAP_USR_IO);

	uart_init(PLATFORM_CONSOLE_VBASE);
}

/*
 * We are using TIMER0 only, so we map TIMER0 base,
 * incase any other timer is needed we need to map it
 * to userspace or kernel space as needed
 */
void init_platform_timer(void)
{
	add_boot_mapping(PLATFORM_TIMER0_BASE, PLATFORM_TIMER0_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);

	/* 1 Mhz means can tick up to 1,000,000 times a second */
	timer_init(PLATFORM_TIMER0_VBASE, 1000000 / CONFIG_SCHED_TICKS);
}

void init_platform_irq_controller()
{
	add_boot_mapping(PLATFORM_VIC_BASE, PLATFORM_IRQCTRL0_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);
	add_boot_mapping(PLATFORM_SIC_BASE, PLATFORM_IRQCTRL1_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);
	irq_controllers_init();
}

/* Add userspace devices here as you develop their irq handlers */
void init_platform_devices()
{
	/* TIMER23 */
	add_boot_mapping(PLATFORM_TIMER1_BASE, PLATFORM_TIMER1_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);

	/* KEYBOARD - KMI0 */
	add_boot_mapping(PLATFORM_KEYBOARD0_BASE, PLATFORM_KEYBOARD0_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);

	/* MOUSE - KMI1 */
	add_boot_mapping(PLATFORM_MOUSE0_BASE, PLATFORM_MOUSE0_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);

	/* CLCD */
	add_boot_mapping(PLATFORM_CLCD0_BASE, PLATFORM_CLCD0_VBASE,
	                 PAGE_SIZE, MAP_IO_DEFAULT);
}

/* If these bits are off, 32Khz OSC source is used */
#define TIMER3_SCTRL_1MHZ	(1 << 21)
#define TIMER2_SCTRL_1MHZ	(1 << 19)
#define TIMER1_SCTRL_1MHZ	(1 << 17)
#define TIMER0_SCTRL_1MHZ	(1 << 15)

/* Set all timers to use 1Mhz OSC clock */
void init_timer_osc(void)
{
	volatile u32 reg;

	add_boot_mapping(PLATFORM_SYSCTRL_BASE, PLATFORM_SYSCTRL_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);

	reg = read(SP810_SCCTRL);

	write(reg | TIMER0_SCTRL_1MHZ | TIMER1_SCTRL_1MHZ
	      | TIMER2_SCTRL_1MHZ | TIMER3_SCTRL_1MHZ,
	      SP810_SCCTRL);
}

void platform_timer_start(void)
{
	/* Enable irq line for TIMER0 */
	irq_enable(IRQ_TIMER0);

	/* Enable timer */
	timer_start(PLATFORM_TIMER0_VBASE);
}

void platform_init(void)
{
	init_timer_osc();
	init_platform_console();
	init_platform_timer();
	init_platform_irq_controller();
	init_platform_devices();
}

