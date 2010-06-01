/*
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
#include INC_GLUE(memlayout.h)
#include INC_PLAT(offsets.h)
#include INC_PLAT(platform.h)
#include INC_PLAT(irq.h)
#include INC_ARCH(asm.h)
#include INC_GLUE(mapping.h)
#include <l4/generic/capability.h>
#include <l4/generic/cap-types.h>
#include <l4/drivers/irq/gic/gic.h>

struct platform_mem_regions platform_mem_regions = {
	.nregions = 3,
	.mem_range = {
		[0] = {
		.start = PLATFORM_PHYS_MEM_START,
		.end = PLATFORM_PHYS_MEM_END,
		.type = MEM_TYPE_RAM,
		},
		[1] = {
		.start = PLATFORM_DEVICES_START,
		.end = PLATFORM_DEVICES_END,
		.type = MEM_TYPE_DEV,
		},
		[2] = {
		.start = PLATFORM_DEVICES1_START,
		.end = PLATFORM_DEVICES1_END,
		.type = MEM_TYPE_DEV,
		},
	},
};

void init_platform_irq_controller()
{
	/* TODO: we need to map 64KB ?*/
	add_boot_mapping(MPCORE_PRIVATE_BASE, MPCORE_PRIVATE_VBASE,
			 PAGE_SIZE * 2, MAP_IO_DEFAULT);

	gic_dist_init(0, GIC0_DIST_VBASE);
	gic_cpu_init(0, GIC0_CPU_VBASE);
	irq_controllers_init();
}

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

	/* System registers */
	add_boot_mapping(PLATFORM_SYSTEM_REGISTERS, PLATFORM_SYSREGS_VBASE,
			 PAGE_SIZE, MAP_IO_DEFAULT);
}

