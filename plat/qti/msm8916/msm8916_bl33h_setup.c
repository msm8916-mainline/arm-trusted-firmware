/*
 * Copyright (c) 2021-2023, Stephan Gerhold <stephan@gerhold.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <common/debug.h>
#include <plat/common/platform.h>

#include "msm8916_setup.h"

static struct {
	entry_point_info_t bl33;
} image_ep_info = {
	/* BL33 entry point */
	SET_STATIC_PARAM_HEAD(bl33, PARAM_EP, VERSION_1,
			      entry_point_info_t, NON_SECURE),
	/*
	.bl33.pc = PRELOADED_BL33_BASE,
	.bl33.spsr = SPSR_MODE32(MODE32_svc, SPSR_T_ARM, SPSR_E_LITTLE,
				 DISABLE_ALL_EXCEPTIONS),
	*/
};

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	entry_point_info_t *bl33 = &image_ep_info.bl33;

	msm8916_early_platform_setup();

	bl33->pc = arg0;
	switch (arg1) {
	case 1:
		bl33->spsr = SPSR_MODE32(MODE32_svc, SPSR_T_ARM, SPSR_E_LITTLE,
					 DISABLE_ALL_EXCEPTIONS);
		break;
	case 2:
		bl33->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
		break;
	default:
		assert(false);
	}
}

void bl31_plat_arch_setup(void)
{
	msm8916_plat_arch_setup(BL33H_BASE, BL33H_END - BL33H_BASE);
	enable_mmu_el2(0);
}

void bl31_platform_setup(void)
{
	INFO("BL33H: Platform setup start\n");
	msm8916_platform_setup();
	INFO("BL33H: Platform setup done\n");
}

entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	assert(type == NON_SECURE);
	return &image_ep_info.bl33;
}
