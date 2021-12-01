/*
 * Copyright (c) 2021-2022, Stephan Gerhold <stephan@gerhold.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/cci.h>
#include <drivers/arm/gicv2.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <plat/common/platform.h>

#include "msm8916_gicv2.h"
#include <msm8916_mmap.h>
#include "msm8916_pm.h"

/*
 * On platforms with two clusters the index of the APCS memory region is swapped
 * compared to the MPIDR cluster affinity level: APCS cluster 0 manages CPUs
 * with cluster affinity level 1, while APCS cluster 1 manages CPUs with level 0.
 *
 * On platforms with a single cluster there is only one APCS memory region.
 */
#if PLATFORM_CLUSTER_COUNT == 2
#define MPIDR_APCS_CLUSTER(mpidr)	!MPIDR_AFFLVL1_VAL(mpidr)
#else
#define MPIDR_APCS_CLUSTER(mpidr)	0
#endif

#define CLUSTER_PWR_STATE(state) ((state)->pwr_domain_state[MPIDR_AFFLVL1])

static int msm8916_pwr_domain_on(u_register_t mpidr)
{
	/* Should be never called on single-core platforms */
	if (PLATFORM_CORE_COUNT == 1) {
		assert(false);
		return PSCI_E_ALREADY_ON;
	}

	if (!msm8916_gicv2_wake_cpu(mpidr)) {
		/* Power on L2 cache and secondary CPU core for the first time */
		if (PLATFORM_CLUSTER_COUNT > 1) {
			msm8916_l2_boot(APCS_GLB(MPIDR_APCS_CLUSTER(mpidr)));
		}
		msm8916_cpu_boot(APCS_ALIAS_ACS(MPIDR_APCS_CLUSTER(mpidr),
						MPIDR_AFFLVL0_VAL(mpidr)));
	}
	return PSCI_E_SUCCESS;
}

static void msm8916_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	/* Should be never called on single-core platforms */
	if (PLATFORM_CORE_COUNT == 1) {
		assert(false);
		return;
	}

	if (PLATFORM_CLUSTER_COUNT > 1 &&
	    CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE) {
		cci_enable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(read_mpidr_el1()));
	}

	msm8916_gicv2_cpu_finish_on();
	gicv2_cpuif_enable();
}

/* Seems to only exist on single core platforms */
#define APCS_CFG_RSTS		(APCS_CFG(0) + 0x14)
#define APCS_CFG_L2RSTDISABLE	BIT_32(11)

static void msm8916_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	gicv2_cpuif_disable();
	msm8916_spm_cpu_set_pc(APCS_BANKED_SAW2);

	/* Skip reset of L2 cache if it is being kept on */
	if (PLATFORM_CORE_COUNT == 1) {
		bool l2_off = CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE;
		mmio_write_32(APCS_CFG_RSTS, l2_off ? 0 : APCS_CFG_L2RSTDISABLE);
	}
}

static void msm8916_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	gicv2_cpuif_enable();
}

static int msm8916_validate_power_state(unsigned int power_state,
					psci_power_state_t *req_state)
{
	/*
	 * Standby states are not supported at the moment,
	 * the non-secure world can just execute WFI directly.
	 */
	if (psci_get_pstate_type(power_state) != PSTATE_TYPE_POWERDOWN) {
		return PSCI_E_INVALID_PARAMS;
	}

	/* Only CPU power down is supported at the moment */
	if (psci_get_pstate_id(power_state) != PLAT_MAX_OFF_STATE) {
		return PSCI_E_INVALID_PARAMS;
	}

	req_state->pwr_domain_state[MPIDR_AFFLVL0] = PLAT_MAX_OFF_STATE;
	return PSCI_E_SUCCESS;
}

static void msm8916_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	unsigned int lvl;

	for (lvl = MPIDR_AFFLVL0; lvl <= PLAT_MAX_PWR_LVL; lvl++) {
		req_state->pwr_domain_state[lvl] = PLAT_MAX_OFF_STATE;
	}
}

static void __dead2 msm8916_system_reset(void)
{
	mmio_write_32(MPM_PS_HOLD, 0);
	mdelay(1000);

	ERROR("PSCI: System reset failed\n");
	panic();
}

static const plat_psci_ops_t msm8916_psci_ops = {
	.pwr_domain_on			= msm8916_pwr_domain_on,
	.pwr_domain_on_finish		= msm8916_pwr_domain_on_finish,
	.pwr_domain_off			= msm8916_pwr_domain_suspend,
	.pwr_domain_suspend		= msm8916_pwr_domain_suspend,
	.pwr_domain_suspend_finish	= msm8916_pwr_domain_suspend_finish,
	.validate_power_state		= msm8916_validate_power_state,
	.get_sys_suspend_power_state	= msm8916_get_sys_suspend_power_state,
	.system_off			= msm8916_system_reset,
	.system_reset			= msm8916_system_reset,
};

/* Defined and used in msm8916_helpers.S */
extern uintptr_t msm8916_entry_point;

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	if (PLATFORM_CORE_COUNT > 1) {
		unsigned int cluster, cpu;

		for (cluster = 0; cluster < PLATFORM_CLUSTER_COUNT; cluster++) {
			for (cpu = 0; cpu < PLATFORM_CPUS_PER_CLUSTER; cpu++) {
				msm8916_spm_cpu_init(APCS_ALIAS_SAW2(cluster, cpu));
			}
		}
	} else {
		/* There is just one core so no aliases exist */
		msm8916_spm_cpu_init(APCS_BANKED_SAW2);
	}

	/*
	 * The entry point is read with caches off (and even from two different
	 * physical addresses when read through the "boot remapper"), so make
	 * sure it is flushed to memory.
	 */
	msm8916_entry_point = sec_entrypoint;
	flush_dcache_range((uintptr_t)&msm8916_entry_point, sizeof(uintptr_t));

	*psci_ops = &msm8916_psci_ops;
	return 0;
}
