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
#include INC_PLAT(printascii.h)
#include INC_SUBARCH(mm.h)
#include INC_SUBARCH(mmu_ops.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(memlayout.h)
#include INC_PLAT(offsets.h)
#include INC_PLAT(platform.h)
#include INC_PLAT(uart.h)
#include INC_PLAT(irq.h)
#include INC_ARCH(asm.h)

/*
 * The devices that are used by the kernel are mapped
 * independent of these capabilities, but these provide a
 * concise description of what is used by the kernel.
 */
int platform_setup_device_caps(struct kernel_resources *kres)
{
	struct capability *uart[4], *timer[2];

	/* Setup capabilities for userspace uarts and timers */
	uart[1] =  alloc_bootmem(sizeof(*uart[1]), 0);
	uart[1]->start = __pfn(PB926_UART1_BASE);
	uart[1]->end = uart[1]->start + 1;
	uart[1]->size = uart[1]->end - uart[1]->start;
	cap_set_devtype(uart[1], CAP_DEVTYPE_UART);
	cap_set_devnum(uart[1], 1);
	link_init(&uart[1]->list);
	cap_list_insert(uart[1], &kres->devmem_free);

	uart[2] =  alloc_bootmem(sizeof(*uart[2]), 0);
	uart[2]->start = __pfn(PB926_UART2_BASE);
	uart[2]->end = uart[2]->start + 1;
	uart[2]->size = uart[2]->end - uart[2]->start;
	cap_set_devtype(uart[2], CAP_DEVTYPE_UART);
	cap_set_devnum(uart[2], 2);
	link_init(&uart[2]->list);
	cap_list_insert(uart[2], &kres->devmem_free);

	uart[3] =  alloc_bootmem(sizeof(*uart[3]), 0);
	uart[3]->start = __pfn(PB926_UART3_BASE);
	uart[3]->end = uart[3]->start + 1;
	uart[3]->size = uart[3]->end - uart[3]->start;
	cap_set_devtype(uart[3], CAP_DEVTYPE_UART);
	cap_set_devnum(uart[3], 3);
	link_init(&uart[3]->list);
	cap_list_insert(uart[3], &kres->devmem_free);

	/* Setup timer1 capability as free */
	timer[1] =  alloc_bootmem(sizeof(*timer[1]), 0);
	timer[1]->start = __pfn(PB926_TIMER23_BASE);
	timer[1]->end = timer[1]->start + 1;
	timer[1]->size = timer[1]->end - timer[1]->start;
	cap_set_devtype(timer[1], CAP_DEVTYPE_TIMER);
	cap_set_devnum(timer[1], 1);
	link_init(&timer[1]->list);
	cap_list_insert(timer[1], &kres->devmem_free);

	return 0;
}

/* We will use UART0 for kernel as well as user tasks, so map it to kernel and user space */
void init_platform_console(void)
{
	add_boot_mapping(PB926_UART0_BASE, PLATFORM_CONSOLE0_BASE, PAGE_SIZE,
			 MAP_IO_DEFAULT_FLAGS);

	/*
	 * Map same UART IO area to userspace so that primitive uart-based
	 * userspace printf can work. Note, this raw mapping is to be
	 * removed in the future, when file-based io is implemented.
	 */
	add_boot_mapping(PB926_UART0_BASE, USERSPACE_UART_BASE, PAGE_SIZE,
			 MAP_USR_IO_FLAGS);

	uart_init();
}

/*
 * We are using TIMER0 only, so we map TIMER0 base,
 * incase any other timer is needed we need to map it
 * to userspace or kernel space as needed
 */
void init_platform_timer(void)
{
	add_boot_mapping(PB926_TIMER01_BASE, PLATFORM_TIMER0_BASE, PAGE_SIZE,
			 MAP_IO_DEFAULT_FLAGS);

	add_boot_mapping(PB926_SYSCTRL_BASE, PB926_SYSCTRL_VBASE, PAGE_SIZE,
			 MAP_IO_DEFAULT_FLAGS);

	timer_init();
}

void init_platform_irq_controller()
{
	add_boot_mapping(PB926_VIC_BASE, PLATFORM_IRQCTRL_BASE, PAGE_SIZE,
			 MAP_IO_DEFAULT_FLAGS);
	add_boot_mapping(PB926_SIC_BASE, PLATFORM_SIRQCTRL_BASE, PAGE_SIZE,
			 MAP_IO_DEFAULT_FLAGS);
	irq_controllers_init();
}

void platform_init(void)
{
	init_platform_console();
	init_platform_timer();
	init_platform_irq_controller();
}

