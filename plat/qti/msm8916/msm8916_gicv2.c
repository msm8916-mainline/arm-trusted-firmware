/*
 * Copyright (c) 2021, Stephan Gerhold <stephan@gerhold.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <common/debug.h>
#include <drivers/arm/gicv2.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>

#include "msm8916_gicv2.h"
#include <msm8916_mmap.h>
#include <platform_def.h>

#define IRQ_SEC_SGI_0		8
#define IRQ_SEC_SGI_1		9
#define IRQ_SEC_SGI_2		10
#define IRQ_SEC_SGI_3		11
#define IRQ_SEC_SGI_4		12
#define IRQ_SEC_SGI_5		13
#define IRQ_SEC_SGI_6		14
#define IRQ_SEC_SGI_7_WAKE_CPU	15	/* Used to wake up CPUs after hotplug */

static const interrupt_prop_t msm8916_interrupt_props[] = {
	INTR_PROP_DESC(IRQ_SEC_SGI_0, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_SGI_6, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_SGI_7_WAKE_CPU, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(IRQ_SEC_PHY_TIMER, GIC_HIGHEST_SEC_PRIORITY,
		       GICV2_INTR_GROUP0, GIC_INTR_CFG_LEVEL),
};

static unsigned int msm8916_target_masks[PLATFORM_CORE_COUNT];
static const gicv2_driver_data_t msm8916_gic_data = {
	.gicd_base		= APCS_QGIC2_GICD,
	.gicc_base		= APCS_QGIC2_GICC,
	.target_masks		= msm8916_target_masks,
	.target_masks_num	= ARRAY_SIZE(msm8916_target_masks),
	.interrupt_props	= msm8916_interrupt_props,
	.interrupt_props_num	= ARRAY_SIZE(msm8916_interrupt_props),
};

void msm8916_gicv2_init(void)
{
	gicv2_driver_init(&msm8916_gic_data);
}

void msm8916_gicv2_configure(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_set_pe_target_mask(plat_my_core_pos());
	gicv2_cpuif_enable();
}

bool msm8916_gicv2_wake_cpu(u_register_t mpidr)
{
	unsigned int core = plat_core_pos_by_mpidr(mpidr);

	assert(core >= 0);

	/* Can only wake CPU through GIC if it has already booted before */
	if (!msm8916_target_masks[core]) {
		return false;
	}

	/* Send SGI to CPU to let SPM power it back up */
	VERBOSE("GICv2: Waking up CPU%d\n", core);
	gicv2_raise_sgi(IRQ_SEC_SGI_7_WAKE_CPU, false, core);
	return true;
}

static void gicd_clear_sgi(uintptr_t base, unsigned int id)
{
	/* Each GICD_CPENDSGIR register contains 4 SGIs from 8 possible CPUs */
	mmio_write_32(base + GICD_CPENDSGIR + (id / 4 * sizeof(uint32_t)),
		      GIC_TARGET_CPU_MASK << (id % 4 * 8));
}

void msm8916_gicv2_cpu_finish_on(void)
{
	unsigned int core = plat_my_core_pos();

	if (msm8916_target_masks[core]) {
		/*
		 * Clear the pending SGI before enabling the GIC CPU interface.
		 * This avoids having to handle it through an interrupt handler
		 * later since we already know that it's pending if we got here.
		 */
		gicd_clear_sgi(APCS_QGIC2_GICD, IRQ_SEC_SGI_7_WAKE_CPU);
	} else {
		/* First boot of the CPU */
		gicv2_pcpu_distif_init();
		gicv2_set_pe_target_mask(core);
	}
}
