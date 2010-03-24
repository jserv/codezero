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

/*
 * The devices that are used by the kernel are mapped
 * independent of these capabilities, but these provide a
 * concise description of what is used by the kernel.
 */
int platform_setup_device_caps(struct kernel_resources *kres)
{
	struct capability *timer[2];

	/* Setup timer1 capability as free */
	timer[1] =  alloc_bootmem(sizeof(*timer[1]), 0);
	timer[1]->start = __pfn(PLATFORM_TIMER1_BASE);
	timer[1]->end = timer[1]->start + 1;
	timer[1]->size = timer[1]->end - timer[1]->start;
	cap_set_devtype(timer[1], CAP_DEVTYPE_TIMER);
	cap_set_devnum(timer[1], 1);
	link_init(&timer[1]->list);
	cap_list_insert(timer[1], &kres->devmem_free);

	return 0;
}

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
}

